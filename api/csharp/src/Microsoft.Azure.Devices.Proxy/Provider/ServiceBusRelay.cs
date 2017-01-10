﻿// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.Azure.Devices.Proxy.Provider {

    using System;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Collections.Concurrent;
    using Relay;
    using Model;

    /// <summary>
    /// A stream service built on top of service bus hybrid connections Relay service. 
    /// Service bus relay provides a scalable, passthru websocket stream, whereby the 
    /// client library acts as listener, and the proxy as client. This is a different 
    /// pattern than more commonly used by relay, but saves users from standing
    /// up a stateful rendezvous point or scaled out websocket services.
    /// </summary>
    public class ServiceBusRelay : IStreamService, IDisposable {

        private TokenProvider _tokenProvider;
        private Uri _uri;
        private HybridConnectionListener _listener;
        private CancellationTokenSource _cts = new CancellationTokenSource();

        /// <summary>
        /// Specialized implementation of relay based message stream
        /// </summary>
        class RelayStream : IConnection, IMessageStream {
            HybridConnectionStream _stream;

            // An outside buffered or memory stream is needed to avoid
            // message pack to wrap with its own buffered stream which 
            // we cannot flush. (packer seems to be broken in 0.9.0)
#if NET45 || NET46
            BufferedStream _buffered;
#endif
            private ServiceBusRelay _relay;
            private ConcurrentQueue<Task> _taskQueue =
                new ConcurrentQueue<Task>();

            /// <summary>
            /// Receive producer queue to read from
            /// </summary>
            public BlockingCollection<Message> ReceiveQueue { get; } =
                new BlockingCollection<Message>();

            /// <summary>
            /// Stream open completion source
            /// </summary>
            internal TaskCompletionSource<IMessageStream> Tcs { get; private set; } =
                new TaskCompletionSource<IMessageStream>();

            /// <summary>
            /// Link reference
            /// </summary>
            internal Reference LinkId { get; private set; }

            /// <summary>
            /// Whether we were closed
            /// </summary>
            public bool Connected { get; set; } = false;

            /// <summary>
            /// Connection string for connection
            /// </summary>
            public ConnectionString ConnectionString { get; private set; }

            /// <summary>
            /// Constructor
            /// </summary>
            /// <param name="relay"></param>
            /// <param name="linkId"></param>
            /// <param name="connectionString"></param>
            public RelayStream(ServiceBusRelay relay, Reference linkId,
                ConnectionString connectionString) {
                _relay = relay;
                LinkId = linkId;
                ConnectionString = connectionString;
            }

            /// <summary>
            /// Accept this stream
            /// </summary>
            /// <param name="ct"></param>
            /// <returns></returns>
            public Task<IMessageStream> OpenAsync(CancellationToken ct) {
                ct.Register(() => {
                    Tcs.TrySetCanceled();
                });
                return Tcs.Task;
            }

            /// <summary>
            /// Connect the stream to a accepted stream instance
            /// </summary>
            /// <param name="stream"></param>
            internal bool TrySetConnect(HybridConnectionStream stream) {
                _stream = stream;
#if NET45 || NET46
                _buffered = new BufferedStream(_stream);
#endif
                if (!Connected) {
                    Connected = Tcs.TrySetResult(this);
                    Tcs = null;
                }
                return Connected;
            }

            /// <summary>
            /// Close stream
            /// </summary>
            /// <returns></returns>
            public async Task CloseAsync() {
                if (_stream == null)
                    return;
                try {
                    await _stream.ShutdownAsync(CancellationToken.None);
                    await _stream.CloseAsync(CancellationToken.None);
                }
                catch(Exception e) {
                    throw ProxyEventSource.Log.Rethrow(e, this);
                }
                finally {
                    try {
                        _stream.Dispose();
                    }
                    catch (Exception) {}
                    finally {
                        _stream = null;
                    }
#if NET45 || NET46
                    try {
                        _buffered.Dispose();
                    }
                    catch (Exception) {}
                    finally {
                        _buffered = null;
                    }
#endif
                }
            }

            /// <summary>
            /// Queues receives into the task queue then waits for them to complete
            /// or for the cancellation token to be signalled. Ensures that full 
            /// message is read nonetheless, no matter if cancelled.
            /// </summary>
            /// <param name="ct"></param>
            /// <returns></returns>
            public async Task ReceiveAsync(CancellationToken ct) {
                while (true) {
                    try {
                        Task task;
                        if (_taskQueue.TryPeek(out task)) {
                            var t = await Task.WhenAny(task, Task.Delay(-1, ct));
                            if (task == t) {
                                // Read completed
                                await task;
                                _taskQueue.TryDequeue(out task);
                            }
                            else {
                                // Timeout
                                ProxyEventSource.Log.Timeout("Timeout during receive");
                            }
                            break;
                        }
                        else {
                            //
                            // Queue a new receive task if queue is empty.  This will
                            // eventually materialize a message into receive queue, i.e.
                            // providing backpressure, but also making synchronous decode 
                            // call async...
                            //
                            _taskQueue.Enqueue(ReceiveOneAsync());
                        }
                    }
                    catch(AggregateException ae) {
                        ProxyEventSource.Log.HandledExceptionAsInformation(this, ae);
                        foreach (var e in ae.InnerExceptions) {
                            if (!(e is OperationCanceledException))
                                throw ProxyEventSource.Log.Rethrow(e, this);
                        }
                    }
                    catch (OperationCanceledException) {
                        break;
                    }
                    catch (Exception e) {
                        throw ProxyEventSource.Log.Rethrow(e, this);
                    }
                }
            }

            /// <summary>
            /// Receive message and enqueue
            /// </summary>
            /// <returns></returns>
            private async Task ReceiveOneAsync() {
                try {
                    var message = await Message.DecodeAsync(
                        _stream, CodecId.Mpack, _relay._cts.Token).ConfigureAwait(false);
                    ReceiveQueue.Add(message, _relay._cts.Token);
                }
                catch(Exception e) {
                    throw ProxyEventSource.Log.Rethrow(e, this);
                }
            }

            /// <summary>
            /// Send message
            /// </summary>
            /// <param name="message"></param>
            /// <param name="ct"></param>
            /// <returns></returns>
            public async Task SendAsync(Message message, CancellationToken ct) {
                try {
#if NET45 || NET46
                    await message.EncodeAsync(_buffered, CodecId.Mpack, ct);
                    await _buffered.FlushAsync();
#else
                    // Poor man's buffered stream.  
                    var stream = new MemoryStream();
                    message.Encode(stream, CodecId.Mpack);
                    var buffered = stream.ToArray();

                    await _stream.WriteAsync(buffered, 0, buffered.Length, ct);
                    await _stream.FlushAsync(ct);
#endif
                }
                catch (Exception e) {
                    throw ProxyEventSource.Log.Rethrow(e, this);
                }
            }
        }

        ConcurrentDictionary<Reference, RelayStream> _streamMap =
            new ConcurrentDictionary<Reference, RelayStream>();

        /// <summary>
        /// Constructor to create service bus relay
        /// </summary>
        /// <param name="hcName">Uri of connection in namespace</param>
        /// <param name="provider">Shared access token provider</param>
        private ServiceBusRelay(Uri hcUri, TokenProvider provider) {
            _tokenProvider = provider;
            _uri = hcUri;
        }

        /// <summary>
        /// Create stream service by accessing the namespace.  If the 
        /// connection with the given name does not exist, it is created.
        /// Using the key it creates a token provider to instantiate the
        /// relay service.
        /// </summary>
        /// <param name="name">Name of the listener connection</param>
        /// <param name="connectionString">Relay root connection string</param>
        /// <returns></returns>
        public static async Task<IStreamService> CreateAsync(string name, 
            ConnectionString connectionString) {
            var ns  = new ServiceBusNamespace(connectionString);
            var key = await ns.GetConnectionKeyAsync(name);
            var relay = new ServiceBusRelay(ns.GetConnectionUri(name), 
                TokenProvider.CreateSharedAccessSignatureTokenProvider("proxy", key));
            await relay.OpenAsync();
            return relay;
        }

        /// <summary>
        /// Creates connection
        /// </summary>
        /// <param name="linkId"></param>
        /// <param name="timeout"></param>
        /// <returns></returns>
        public async Task<IConnection> CreateConnectionAsync(
            Reference linkId, TimeSpan timeout) {
            var uri = new UriBuilder(_uri);
            uri.Scheme = "http";
            var token = await _tokenProvider.GetTokenAsync(uri.ToString(), timeout);
            var connection = new RelayStream(
                this, linkId, new ConnectionString(_uri, "proxy", token));
            _streamMap.AddOrUpdate(linkId, connection, (r, s) => connection);
            return connection;
        }

        /// <summary>
        /// Opens the listener and starts listening thread
        /// </summary>
        /// <returns></returns>
        private async Task OpenAsync() {
            _listener = new HybridConnectionListener(_uri, _tokenProvider);

            // Subscribe to the status events
            _listener.Connecting += (o, e) => { Console.WriteLine("Connecting"); };
            _listener.Offline += (o, e) => { Console.WriteLine("Offline"); };
            _listener.Online += (o, e) => { Console.WriteLine("Online"); };

            await _listener.OpenAsync(_cts.Token);
            _cts.Token.Register(() => { var x = _listener.CloseAsync(CancellationToken.None); });
            var task = RunAsync(); // Call dispose to cancel
            ProxyEventSource.Log.LocalListenerStarted(this);
        }

        /// <summary>
        /// Run listener
        /// </summary>
        /// <param name="ct"></param>
        /// <returns></returns>
        private async Task RunAsync() {
            while (!_cts.IsCancellationRequested) {
                var relayConnection = await _listener.AcceptConnectionAsync();
                if (relayConnection == null) {
                    await _listener.CloseAsync(_cts.Token);
                    ProxyEventSource.Log.LocalListenerClosed(this);
                    break;
                }
                else {
                    DoHandshakeAsync(relayConnection, _cts.Token);
                }
            }
        }

        public void Dispose() {
            _cts.Cancel();
        }

        /// <summary>
        /// Perform handshake
        /// </summary>
        /// <param name="relayConnection"></param>
        /// <param name="ct"></param>
        private async void DoHandshakeAsync(HybridConnectionStream relayConnection, 
            CancellationToken ct) {
            try {
                bool handshakeCompleted = false;
                //
                // Accept connection.  The first message will be an open response, which
                // is sent as handshake on both the remoting side, as well as to open the 
                // stream.  This will allow us to connect the accepted connection to an
                // internal stream without knowing more about the client (since we cannot 
                // see the header and have no clue about auth.  
                //
                try {
                    var message = await Message.DecodeAsync(relayConnection, CodecId.Mpack, ct);
                    RelayStream stream;
                    if (_streamMap.TryGetValue(message.Target, out stream)) {

                        handshakeCompleted = stream.TrySetConnect(relayConnection);
                        if (handshakeCompleted) {
                            ProxyEventSource.Log.StreamAcceptedAndConnected(message.Target);
                            return;
                        }
                        else {
                            ProxyEventSource.Log.StreamAcceptedNotConnected(message.Target);
                        }
                    }
                    else {
                        ProxyEventSource.Log.StreamRejected(message.Target);
                    }

                    // Socket was already disconnected, or accept was cancelled, shutdown...
                    await relayConnection.ShutdownAsync(ct);
                }
                catch (IOException ioex) {
                    // Remote side closed, log ...
                    ProxyEventSource.Log.RemoteProxyClosed(this, ioex);
                }
                catch (Exception e) {
                    // Another error occurred
                    ProxyEventSource.Log.RemoteProxyClosed(this, e);
                }
                finally {
                    if (!handshakeCompleted) {
                        // ... and close the connection
                        await relayConnection.CloseAsync(ct);
                    }
                }
            }
            catch (Exception e) {
                // Another exception occurred, catch to not crash...
                ProxyEventSource.Log.Rethrow(e, this, System.Diagnostics.Tracing.EventLevel.Warning);
            }
        }
    }
}
