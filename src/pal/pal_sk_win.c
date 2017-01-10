// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "util_mem.h"
#include "util_string.h"
#include "pal.h"
#include "pal_sk.h"
#include "pal_net.h"
#include "pal_mt.h"
#include "pal_types.h"
#include "pal_err.h"
#include "os_win.h"

static LPFN_CONNECTEX _ConnectEx = NULL;
static LPFN_ACCEPTEX _AcceptEx = NULL;
static LPFN_GETACCEPTEXSOCKADDRS _GetAcceptExSockAddrs = NULL;

//
// Io Completion port operation context
//
typedef struct pal_socket_async pal_socket_async_t;

//
// Begin operation - different per socket type 
//
typedef bool (*pal_socket_async_begin_t)(
    pal_socket_async_t* sock
    );

//
// Complete operation - different per socket type
//
typedef void (*pal_socket_async_complete_t)(
    pal_socket_async_t* async_op,
    int32_t result,
    size_t len
    );

//
// Io Completion port operation context
//
struct pal_socket_async
{
    OVERLAPPED ov;        // Must be first to cast from OVERLAPPED*
    pal_socket_t* sock;
    atomic_t pending;       // Whether the operation is in progress
    pal_socket_async_begin_t begin;              // Begin operation
    pal_socket_async_complete_t complete;     // Complete operation 
    DWORD flags;                           // Flags set or returned
    SOCKADDR_STORAGE addr_buf[2]; // Socket address buf for this op
    socklen_t addr_len;      // length of address in address buffer
    uint8_t* buffer;
    size_t buf_len;
    void* context;
};

//
// Represents a async winsock socket using io completion ports
//
struct pal_socket
{
    pal_socket_client_itf_t itf;                // Client interface
    SOCKET sock_fd;                // Real underlying socket handle
                     
    prx_addrinfo_t* prx_ai;   // For async connect save the ai result
    prx_size_t prx_ai_count;        // Size of the resolved addresses
    prx_size_t prx_ai_cur;              // Current address to connect

    pal_socket_async_t open_op;          // Async connect operation
    pal_socket_async_t send_op;             // Async send operation
    pal_socket_async_t recv_op;   // Async recv or accept operation

    prx_socket_address_t local;              // Cached local address
    prx_socket_address_t peer;                // Cached peer address
    void* close_context;
    log_t log;
};

//
// Get error code for os error
//
static int32_t pal_socket_from_os_error(
    DWORD error
)
{
    char* message = NULL;
    /**/ if (error == ERROR_SUCCESS)
        return er_ok;
    else if (error != STATUS_CANCELLED)
    {
        FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (char*)&message, 0, NULL);

        log_error(NULL, "Socket error code 0x%x: %s",
            error, message ? message : "<unknown>");
        LocalFree(message);
    }
    return pal_os_to_prx_error(error);
}

//
// Check whether all operations are complete while socket is closing
//
static void pal_socket_close_complete(
    pal_socket_t* sock
)
{
    dbg_assert_ptr(sock);
    void* context = sock->close_context;

    if (context &&
        !sock->open_op.pending &&
        !sock->recv_op.pending &&
        !sock->send_op.pending)
    {
        sock->close_context = NULL;

        if (sock->sock_fd != INVALID_SOCKET)
        {
            closesocket(sock->sock_fd);
            sock->sock_fd = INVALID_SOCKET;
        }

        // No more operations are pending, close
        sock->itf.cb(sock->itf.context, pal_socket_event_closed, NULL,
            NULL, NULL, NULL, er_ok, &context);
    }
}

//
// Called when open completes - terminal state for begin open
//
static void pal_socket_open_complete(
    pal_socket_t* sock,
    int32_t result,
    void* op_context
)
{
    dbg_assert_ptr(sock);

    // Complete open
    sock->itf.cb(sock->itf.context, pal_socket_event_opened, NULL,
        NULL, NULL, NULL, result, &op_context);

    if (!sock->prx_ai)
        return;

    pal_freeaddrinfo(sock->prx_ai);
    sock->prx_ai = NULL;
    sock->prx_ai_count = 0;
    sock->prx_ai_cur = 0;
}

//
// Called when ConnectEx completes in any mode
//
static int32_t pal_socket_connect_complete(
    pal_socket_t* sock,
    int32_t result,
    size_t len
)
{
    (void)len;
    int error;
    dbg_assert_ptr(sock);

    while(result == er_ok)
    {
        // Update the connect context.
        error = setsockopt(sock->sock_fd, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT,
            NULL, 0);
        if (error != 0)
        {
            result = pal_os_last_net_error_as_prx_error();
            break;
        }

        sock->open_op.addr_len = sizeof(sock->open_op.addr_buf);
        error = getsockname(sock->sock_fd,
            (struct sockaddr*)sock->open_op.addr_buf, &sock->open_op.addr_len);
        if (error != 0)
        {
            result = pal_os_last_net_error_as_prx_error();
            break;
        }
        result = pal_os_to_prx_socket_address(
            (const struct sockaddr*)sock->open_op.addr_buf, 
            sock->open_op.addr_len, &sock->local);
        if (result != er_ok)
            break;

        sock->open_op.addr_len = sizeof(sock->open_op.addr_buf);
        error = getpeername(sock->sock_fd,
            (struct sockaddr*)sock->open_op.addr_buf, &sock->open_op.addr_len);
        if (error != 0)
        {
            result = pal_os_last_net_error_as_prx_error();
            break;
        }
        result = pal_os_to_prx_socket_address(
            (const struct sockaddr*)sock->open_op.addr_buf,
            sock->open_op.addr_len, &sock->peer);
        break;
    }

    sock->open_op.addr_len = 0;

    if (result != er_ok && sock->sock_fd != INVALID_SOCKET)
    {
        closesocket(sock->sock_fd);
        sock->sock_fd = INVALID_SOCKET;
    }
    return result;
}

//
// No-op callback
//
static bool pal_socket_async_no_op(
    pal_socket_async_t* async_op
)
{
    (void)async_op;
    return false;
}

//
// Io completion port operation callback when operation completed
//
static void CALLBACK pal_socket_async_complete_from_OVERLAPPED(
    DWORD error,
    DWORD bytes,
    LPOVERLAPPED ov
)
{
    pal_socket_async_t* async_op = (pal_socket_async_t*)ov;
    dbg_assert_ptr(async_op);
    dbg_assert_ptr(async_op->complete);
    atomic_inc(async_op->pending);

    async_op->complete(async_op, pal_socket_from_os_error(error), (size_t)bytes);
        
    // Continue loop until user fails to submit buffers or op is shutdown
    while (async_op->begin(async_op))
        ;
    atomic_dec(async_op->pending);
}

//
// Continue open session, opens next address in list or completes
//
static void pal_socket_open_next_begin(
    pal_socket_t *sock,
    void* context
);

//
// Called when ConnectEx completes asynchronously
//
static void pal_socket_async_connect_complete(
    pal_socket_async_t* async_op,
    int32_t result,
    size_t len
)
{
    dbg_assert_ptr(async_op);
    dbg_assert_ptr(async_op->sock);
    // Complete connection
    result = pal_socket_connect_complete(async_op->sock, result, len);
    if (result == er_ok)
    {
        pal_socket_open_complete(async_op->sock, er_ok, async_op->context);

        // Success!
        log_info(async_op->sock->log, 
            "Socket connected asynchronously!");
    } 
    else
    {
        log_error(async_op->sock->log, 
            "Failed to connect socket, continue... (%s)",
            prx_err_string(result));

        // Continue with next address
        pal_socket_open_next_begin(async_op->sock, async_op->context);
    }
    async_op->buffer = NULL;
    async_op->buf_len = 0;
    async_op->addr_len = 0;
    async_op->context = NULL;
    atomic_dec(async_op->pending);
}

//
// Called when AcceptEx completes
//
static void pal_socket_async_accept_complete(
    pal_socket_async_t* async_op,
    int32_t result,
    size_t len
)
{
    pal_socket_t* accepted = (pal_socket_t*)async_op->buffer;
    struct sockaddr* sa_local, *sa_peer;
    socklen_t sa_llen, sa_plen;
    dbg_assert_ptr(async_op);
    dbg_assert(async_op->buf_len == sizeof(pal_socket_t*));
    dbg_assert_ptr(async_op->sock);
    do
    {
        if (result != er_ok)
        {
            log_error(async_op->sock->log,
                "Failed accept (%s)", prx_err_string(result));
            break;
        }

        // Parse addresses
        _GetAcceptExSockAddrs(async_op->addr_buf, 0,
            sizeof(async_op->addr_buf[0]),
            sizeof(async_op->addr_buf[1]),
            &sa_local, &sa_llen, &sa_peer, &sa_plen);

        result = pal_os_to_prx_socket_address(sa_local, sa_llen, &accepted->local);
        if (result != er_ok)
        {
            log_error(async_op->sock->log, "Accept received bad local address (%s)",
                prx_err_string(result));
            break;
        }

        result = pal_os_to_prx_socket_address(sa_peer, sa_plen, &accepted->peer);
        if (result != er_ok)
        {
            log_error(async_op->sock->log, "Accept received bad peer address (%s)",
                prx_err_string(result));
            break;
        }

        // Update props
        memcpy(&accepted->itf.props.address, &accepted->peer, 
            sizeof(prx_socket_address_t));
        accepted->itf.props.family = accepted->peer.un.family;

        // Update the accept context.
        if (0 != setsockopt(accepted->sock_fd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
            (char*)&accepted->sock_fd, sizeof(SOCKET)))
        {
            result = pal_os_last_net_error_as_prx_error();
            break;
        }
    } 
    while (0);

    // Complete accept with the new socket as the argument.
    len = sizeof(pal_socket_t*);
    async_op->sock->itf.cb(async_op->sock->itf.context, pal_socket_event_end_accept,
        (uint8_t**)&accepted, &len, NULL, NULL, result, &async_op->context);

    if (result != er_ok)
    {
        if (accepted->sock_fd != INVALID_SOCKET)
        {
            closesocket(accepted->sock_fd);
            accepted->sock_fd = INVALID_SOCKET;
        }
        pal_socket_free(accepted);
    }
    else
    {
        // Open accepted socket
        pal_socket_open_complete(accepted, er_ok, NULL);
    }
    async_op->buffer = NULL;
    async_op->buf_len = 0;
    async_op->addr_len = 0;
    async_op->context = NULL;
    atomic_dec(async_op->pending);
}

//
// Called when WSASend completes
//
static void pal_socket_async_send_complete(
    pal_socket_async_t* async_op,
    int32_t result,
    size_t sent
)
{
    dbg_assert_ptr(async_op);
    dbg_assert_ptr(async_op->sock);
    // Complete send
    async_op->sock->itf.cb(async_op->sock->itf.context, pal_socket_event_end_send,
        &async_op->buffer, &sent, NULL, NULL, result, &async_op->context);
    async_op->buffer = NULL;
    async_op->buf_len = 0;
    async_op->addr_len = 0;
    async_op->context = NULL;
    atomic_dec(async_op->pending);
}

//
// Called when WSARecv completes
//
static void pal_socket_async_recv_complete(
    pal_socket_async_t* async_op,
    int32_t result,
    size_t received
)
{
    int32_t flags = 0;

    dbg_assert_ptr(async_op);
    dbg_assert_ptr(async_op->sock);
    dbg_assert(!async_op->addr_len, "Expected no adddress on WSARecv");
   
    if (result == er_ok)
    {
        result = pal_os_to_prx_message_flags(async_op->flags, &flags);
        if (result != er_ok)
        {
            log_error(async_op->sock->log, "Recv received bad flags %x (%s)",
                async_op->flags, prx_err_string(result));
        }
    }

    async_op->sock->itf.cb(async_op->sock->itf.context, pal_socket_event_end_recv,
        &async_op->buffer, &received, NULL, &flags, result, &async_op->context);
    async_op->buffer = NULL;
    async_op->buf_len = 0;
    async_op->addr_len = 0;
    async_op->context = NULL;
    atomic_dec(async_op->pending);
}

//
// Called when WSASendTo completes
//
static void pal_socket_async_sendto_complete(
    pal_socket_async_t* async_op,
    int32_t result,
    size_t sent
)
{
    dbg_assert_ptr(async_op);
    dbg_assert_ptr(async_op->sock);
    // Complete send
    async_op->sock->itf.cb(async_op->sock->itf.context, pal_socket_event_end_send,
        &async_op->buffer, &sent, NULL, NULL, result, &async_op->context);
    async_op->buffer = NULL;
    async_op->buf_len = 0;
    async_op->addr_len = 0;
    async_op->context = NULL;
    atomic_dec(async_op->pending);
}

//
// Called when WSARecvFrom completes
//
static void pal_socket_async_recvfrom_complete(
    pal_socket_async_t* async_op,
    int32_t result,
    size_t received
)
{
    int32_t flags = 0;
    prx_socket_address_t addr, *addr_ptr = NULL;
    dbg_assert_ptr(async_op);
    dbg_assert_ptr(async_op->sock);
    do
    {
        if (result != er_ok)
            break;

        // Process received flags and address... 
        result = pal_os_to_prx_message_flags(async_op->flags, &flags);
        if (result != er_ok)
        {
            log_error(async_op->sock->log, "Recvfrom received bad flags %x (%s)",
                async_op->flags, prx_err_string(result));
            break;
        }

        result = pal_os_to_prx_socket_address(
            (struct sockaddr*)async_op->addr_buf, async_op->addr_len, &addr);
        if (result != er_ok)
        {
            log_error(async_op->sock->log, "Recvfrom received bad address (%s)",
                prx_err_string(result));
            break;
        }
        addr_ptr = &addr;
        break;
    } 
    while (0);

    async_op->sock->itf.cb(async_op->sock->itf.context, pal_socket_event_end_recv,
        &async_op->buffer, &received, addr_ptr, &flags, result, &async_op->context);
    async_op->buffer = NULL;
    async_op->buf_len = 0;
    async_op->addr_len = 0;
    async_op->context = NULL;
    atomic_dec(async_op->pending);
}

//
// Create new socket fd that is bound to iocpt
//
static int32_t pal_socket_properties_to_fd(
    prx_socket_properties_t* props,
    SOCKET* sock_fd
)
{
    int32_t result;
    int os_af, os_type, os_proto;
    dbg_assert_ptr(props);
    dbg_assert_ptr(sock_fd);

    result = pal_os_from_prx_address_family(props->family, &os_af);
    if (result != er_ok)
        return result;
    result = pal_os_from_prx_socket_type(props->sock_type, &os_type);
    if (result != er_ok)
        return result;
    result = pal_os_from_prx_protocol_type(props->proto_type, &os_proto);
    if (result != er_ok)
        return result;

    *sock_fd = WSASocket(os_af, os_type, os_proto, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (*sock_fd == INVALID_SOCKET)
        return pal_os_last_net_error_as_prx_error();

    if (!BindIoCompletionCallback(
        (HANDLE)*sock_fd, pal_socket_async_complete_from_OVERLAPPED, 0))
    {
        closesocket(*sock_fd);
        return pal_os_last_net_error_as_prx_error();
    }
    return er_ok;
}

//
// Kicks off the operation on the io completion port if none is pending
//
static void pal_socket_async_begin(
    pal_socket_async_t* async_op
)
{
    if (!async_op->pending)
    {
        // Kick off operation, but only once if not already in progress
        while (async_op->begin(async_op))
            ;
    }
}

//
// Begin accept operation
//
static bool pal_socket_async_accept_begin(
    pal_socket_async_t* async_op
)
{
    int32_t result;
    DWORD received = 0;
    int error;
    pal_socket_t* accepted;
    dbg_assert_ptr(async_op);
    dbg_assert_ptr(async_op->sock);

    // Call receive and get a client socket option
    async_op->sock->itf.cb(async_op->sock->itf.context, pal_socket_event_begin_accept, 
        &async_op->buffer, &async_op->buf_len, NULL, NULL, er_ok, &async_op->context);
    if (!async_op->buffer || async_op->buf_len != sizeof(pal_socket_client_itf_t))
    {
        // Done accepting
        return false;
    }
    do
    {
        atomic_inc(async_op->pending);
        // Create new socket object and handle to accept with
        result = pal_socket_create(
            (pal_socket_client_itf_t*)async_op->buffer, &accepted);
        if (result != er_ok)
        {
            log_error(async_op->sock->log, "Failed to create Socket object. (%s)",
                prx_err_string(result));
            break;
        }

        result = pal_socket_properties_to_fd(
            &async_op->sock->itf.props, &accepted->sock_fd);
        if (result != er_ok)
        {
            log_error(async_op->sock->log, "Failed to create Socket handle. (%s)",
                prx_err_string(result));
            break;
        }

        async_op->buffer = (uint8_t*)accepted; // Now complete accept
        async_op->buf_len = sizeof(accepted);

        if (!_AcceptEx(async_op->sock->sock_fd, accepted->sock_fd, async_op->addr_buf,
            0, sizeof(async_op->addr_buf[0]), sizeof(async_op->addr_buf[1]),
            &received, &async_op->ov))
        {
            error = WSAGetLastError();
            if (error == WSA_IO_PENDING)
            {
                // Wait for callback
                return false;
            }
            result = pal_os_to_prx_error(error);
        }
        else
        {
            // Wait for callback
            return false;
        }
    } 
    while (0);
    // Complete now
    pal_socket_async_accept_complete(async_op, received, result);
    return result == er_ok;
}

//
// Begin send operation
//
static bool pal_socket_async_send_begin(
    pal_socket_async_t* async_op
)
{
    int32_t result;
    DWORD sent = 0;
    int error, os_flags;
    WSABUF buf;
    int32_t flags;
    dbg_assert_ptr(async_op);
    dbg_assert_ptr(async_op->sock);

    async_op->sock->itf.cb(async_op->sock->itf.context, pal_socket_event_begin_send, 
        &async_op->buffer, &async_op->buf_len, NULL, &flags, er_ok, &async_op->context);
    if (!async_op->buffer)
    {
        // Done sending
        return false;
    }
    do
    {
        atomic_inc(async_op->pending);
        result = pal_os_from_prx_message_flags(flags, &os_flags);
        if (result != er_ok)
        {
            log_error(async_op->sock->log, "Send received bad flags %d", os_flags);
            break;
        }

        buf.buf = (char*)async_op->buffer;
        buf.len = (u_long)async_op->buf_len;
 
        error = WSASend(async_op->sock->sock_fd, &buf, 1, &sent, (DWORD)os_flags,
            &async_op->ov, NULL);
        if (error != 0)
        {
            error = WSAGetLastError();
            if (error == WSA_IO_PENDING)
            {
                // Wait for callback
                return false;
            }
            result = pal_os_to_prx_error(error);
        }
        else
        {
            // Wait for callback
            return false;
        }
    } 
    while (0);
    // Complete now
    pal_socket_async_send_complete(async_op, result, sent);
    return result == er_ok;
}

//
// Begin recv operation
//
static bool pal_socket_async_recv_begin(
    pal_socket_async_t* async_op
)
{
    int32_t result;
    DWORD received = 0;
    int error;
    WSABUF buf;
    dbg_assert_ptr(async_op);
    dbg_assert_ptr(async_op->sock);

    async_op->sock->itf.cb(async_op->sock->itf.context, pal_socket_event_begin_recv, 
        &async_op->buffer, &async_op->buf_len, NULL, NULL, er_ok, &async_op->context);
    if (!async_op->buffer)
    {
        // Done recv
        return false;
    }
    do
    {
        atomic_inc(async_op->pending);
        buf.buf = (char*)async_op->buffer;
        buf.len = (u_long)async_op->buf_len;

        error = WSARecv(async_op->sock->sock_fd, &buf, 1, &received, 
            &async_op->flags, &async_op->ov, NULL);
        if (error != 0)
        {
            error = WSAGetLastError();
            if (error == WSA_IO_PENDING)
            {
                // Wait for callback
                return false;
            }
            result = pal_os_to_prx_error(error);
        }
        else
        {
            // Wait for callback
            return false;
        }
    }
    while (0);
    // Complete now
    pal_socket_async_recv_complete(async_op, result, received);
    return result == er_ok;
}

//
// Begin sendto operation
//
static bool pal_socket_async_sendto_begin(
    pal_socket_async_t* async_op
)
{
    int32_t result;
    DWORD sent = 0;
    int error;
    WSABUF buf;
    int32_t flags;
    prx_socket_address_t addr;
    dbg_assert_ptr(async_op);
    dbg_assert_ptr(async_op->sock);

    async_op->sock->itf.cb(async_op->sock->itf.context, pal_socket_event_begin_send,
        &async_op->buffer, &async_op->buf_len, &addr, &flags, er_ok, &async_op->context);
    if (!async_op->buffer)
    {
        // Done sending
        return false;
    }
    do
    {
        atomic_inc(async_op->pending);
        async_op->addr_len = sizeof(async_op->addr_buf);
        result = pal_os_from_prx_socket_address(&addr,
            (struct sockaddr*)async_op->addr_buf, &async_op->addr_len);
        if (result != er_ok)
        {
            log_error(async_op->sock->log, "Sendto received bad address (%s)",
                prx_err_string(result));
            break;
        }

        result = pal_os_from_prx_message_flags(flags, (int*)&async_op->flags);
        if (result != er_ok)
        {
            log_error(async_op->sock->log, "Sendto received bad flags %x (%s)",
                flags, prx_err_string(result));
            break;
        }

        buf.buf = (char*)async_op->buffer;
        buf.len = (u_long)async_op->buf_len;

        error = WSASendTo(async_op->sock->sock_fd, &buf, 1, &sent, async_op->flags,
            (struct sockaddr*)async_op->addr_buf, async_op->addr_len, &async_op->ov, 
            NULL);
        if (error != 0)
        {
            error = WSAGetLastError();
            if (error == WSA_IO_PENDING)
            {
                // Wait for callback
                return false;
            }
        }
        else
        {
            // Wait for callback
            return false;
        }
    }
    while (0);
    // Complete now
    pal_socket_async_send_complete(async_op, result, sent);
    return result == er_ok;
}

//
// Begin recvfrom operation
//
static bool pal_socket_async_recvfrom_begin(
    pal_socket_async_t* async_op
)
{
    int32_t result;
    DWORD received = 0;
    int error;
    WSABUF buf;
    dbg_assert_ptr(async_op);
    dbg_assert_ptr(async_op->sock);

    async_op->sock->itf.cb(async_op->sock->itf.context, pal_socket_event_begin_recv, 
        &async_op->buffer, &async_op->buf_len, NULL, NULL, er_ok, &async_op->context);
    if (!async_op->buffer)
    {
        // Done receiving
        return false;
    }
    do
    {
        atomic_inc(async_op->pending);
        buf.buf = (char*)async_op->buffer;
        buf.len = (u_long)async_op->buf_len;

        async_op->addr_len = sizeof(async_op->addr_buf);
        error = WSARecvFrom(async_op->sock->sock_fd, &buf, 1, &received,
            &async_op->flags, (struct sockaddr*)async_op->addr_buf,
            &async_op->addr_len, &async_op->ov, NULL);
        if (error != 0)
        {
            error = WSAGetLastError();
            if (error == WSA_IO_PENDING)
            {
                // Wait for callback
                return false;
            }
            result = pal_os_to_prx_error(error);
        }
        else
        {
            // Wait for callback
            return false;
        }
    }
    while (0);
    // Complete now
    pal_socket_async_recv_complete(async_op, result, received);
    return result == er_ok;
}

//
// Close begin callback
//
static bool pal_socket_async_close_begin(
    pal_socket_async_t* async_op
)
{
    if (!async_op->pending)
        return false;

    atomic_dec(async_op->pending);
    dbg_assert(!async_op->pending, "pending count is not 0.");

    // If socket is closing, decrement pending count and check all other operations
    dbg_assert_ptr(async_op->sock);
    pal_socket_close_complete(async_op->sock);
   
    atomic_inc(async_op->pending);
    return false;
}

//
// Begin connect operation
//
static int32_t pal_socket_async_connect_begin(
    pal_socket_async_t* async_op
)
{
    int32_t result;
    DWORD error;

    dbg_assert_ptr(async_op);
    dbg_assert_ptr(async_op->sock);

    // Connect socket
    atomic_inc(async_op->pending);
    do
    {
        // Bind to in_addr_any = 0 - mandatory for ConnectEx
        memset(&async_op->addr_buf[1], 0, sizeof(async_op->addr_buf[1]));
        async_op->addr_buf[1].ss_family = async_op->addr_buf[0].ss_family;
        error = bind(async_op->sock->sock_fd,
            (const struct sockaddr*)&async_op->addr_buf[1],
            (int)async_op->addr_len);
        if (error != 0)
        {
            result = pal_os_last_net_error_as_prx_error();
            log_error(async_op->sock->log, "Failed binding socket for connect (%s)",
                prx_err_string(result));
            break;
        }
        
        if (!_ConnectEx(async_op->sock->sock_fd,
            (const struct sockaddr*)async_op->addr_buf, (int)async_op->addr_len,
            NULL, 0, NULL, &async_op->ov))
        {
            error = WSAGetLastError();
            if (error == ERROR_IO_PENDING)
            {
                //
                // Wait for callback, indicate open loop to break out...
                //
                return er_waiting;
            }
            result = pal_os_to_prx_error(error);
            log_error(async_op->sock->log, "Failed connecting socket (%s)",
                prx_err_string(result));
        }
        else
        {
          //  log_info(async_op->sock->log, "Socket connected synchronously!");
          //  result = er_ok;
            return er_waiting;
        }
    } 
    while (0);

    // Finish connect
    result = pal_socket_connect_complete(async_op->sock, result, 0);
    atomic_dec(async_op->pending);
    return result;
}

//
// Begin bind operation
//
static int32_t pal_socket_bind(
    pal_socket_t* sock
)
{
    int32_t result;
    DWORD error;
    dbg_assert_ptr(sock);

    // Passive or listener, can only do synchronous
    atomic_inc(sock->open_op.pending);
    do
    {
        error = bind(sock->sock_fd,
            (const struct sockaddr*)sock->open_op.addr_buf,
            (int)sock->open_op.addr_len);

        if (error != 0)
        {
            result = pal_os_last_net_error_as_prx_error();
            log_error(sock->log, "Failed binding socket (%s)",
                prx_err_string(result));
            break;
        }
        else
        {
            log_info(sock->log, "Socket bound synchronously!");
            result = er_ok;
        }

        if (sock->itf.props.sock_type == prx_socket_type_dgram ||
            sock->itf.props.sock_type == prx_socket_type_raw)
            break;
        
        dbg_assert(0 != (sock->itf.props.flags & socket_flag_passive),
            "should be passive");

        // Start listen immediately
        error = listen(sock->sock_fd, -1);
        if (error != 0)
        {
            result = pal_os_last_net_error_as_prx_error();
            log_error(sock->log, "Failed to set socket to listen (%s)",
                prx_err_string(result));
            break;
        }
 
        log_info(sock->log, "Socket listening...");
        result = er_ok;
    }
    while(0);
    atomic_dec(sock->open_op.pending);
    return result;
}

// 
// Try opening socket based on address in connect op
//
static int32_t pal_socket_open_begin(
    pal_socket_t *sock
)
{
    int32_t result;
    dbg_assert_ptr(sock);

    // Create new iocp socket and attempt to open based on properties
    result = pal_socket_properties_to_fd(&sock->itf.props, &sock->sock_fd);
    if (result != er_ok)
    {
        log_error(sock->log, "Failed creating iocp socket (%s)!",
            prx_err_string(result));
        return result;
    }

    if ((sock->itf.props.sock_type == prx_socket_type_seqpacket ||
         sock->itf.props.sock_type == prx_socket_type_rdm ||
         sock->itf.props.sock_type == prx_socket_type_stream) &&
        !(sock->itf.props.flags & socket_flag_passive))
    {
        // If stream socket, and not passive, then use ConnectEx
        result = pal_socket_async_connect_begin(&sock->open_op);
    }
    else
    {
        // Otherwise, bind and optionally start listening...
        result = pal_socket_bind(sock);
    }

    // Failed synchronously, close socket...
    if (result != er_ok && 
        result != er_waiting && sock->sock_fd != INVALID_SOCKET)
    {
        closesocket(sock->sock_fd);
        sock->sock_fd = INVALID_SOCKET;
    }
    return result;
}

//
// Open socket based on next address in cached list or fail
//
static void pal_socket_open_next_begin(
    pal_socket_t *sock,
    void* op_context
)
{
    int32_t result;
    dbg_assert_ptr(sock);
    
    while (true)
    {
        if (sock->prx_ai_cur >= sock->prx_ai_count)
        {
            log_error(sock->log, "No other candidate addresses to open...");
            result = er_connecting;
            break;
        }

        // Setup async operation structure for open operation
        sock->open_op.context = op_context;
        sock->open_op.addr_len = sizeof(sock->open_op.addr_buf);
        result = pal_os_from_prx_socket_address(&sock->prx_ai[sock->prx_ai_cur].address,
            (struct sockaddr*)sock->open_op.addr_buf, &sock->open_op.addr_len);
        if (result != er_ok)
            break;

        // Update address family in properties
        sock->itf.props.family = sock->prx_ai[sock->prx_ai_cur].address.un.family;

        // Start to open
        result = pal_socket_open_begin(sock);
        if (result == er_waiting)
            return; // Wait for callback

        if (result != er_ok)
        {
            sock->prx_ai_cur++;
            continue;  // Try next
        }
        // Success!
        log_debug(sock->log, "Socket opened synchronously!");
        break;
    }

    // Complete open which frees address list
    pal_socket_open_complete(sock, result, op_context);
}

//
// Resolve proxy address first and try to open each returned address
//
static int32_t pal_socket_open_by_name_begin(
    pal_socket_t *sock,
    void* op_context
)
{
    int32_t result;
    char port[MAX_PORT_LENGTH];
    const char* server = NULL;
    uint32_t flags = 0;

    dbg_assert_ptr(sock);
    do
    {
        dbg_assert(sock->itf.props.address.un.family == prx_address_family_proxy,
            "Bad address family");
        if (strlen(sock->itf.props.address.un.proxy.host) != 0)
            server = sock->itf.props.address.un.proxy.host;

        result = string_from_int(
            sock->itf.props.address.un.ip.port, 10, port, sizeof(port));
        if (result != er_ok)
            break;

        log_info(sock->log, "Resolving %s:%s...",
            server ? server : "<null>", port);
        if (sock->itf.props.flags & socket_flag_passive)
            flags |= prx_ai_passive;
        result = pal_getaddrinfo(
            server, port, sock->itf.props.family, flags, &sock->prx_ai, &sock->prx_ai_count);
        if (result == er_ok && !sock->prx_ai_count)
            result = er_connecting;
        if (result != er_ok)
        {
            log_error(sock->log, "pal_getaddrinfo for %s:%s failed (%s).",
                server ? server : "<null>", port, prx_err_string(result));
            break;
        }

        // Now we have a list of addresses, try to open one by one...
        pal_socket_open_next_begin(sock, op_context);
        result = er_ok;
        break;
    } 
    while (0);
    return result;
}

//
// Open address without resolving name
//
static int32_t pal_socket_open_by_addr_begin(
    pal_socket_t *sock,
    void* op_context
)
{
    int32_t result;
    dbg_assert_ptr(sock);
    do
    {
        dbg_assert(sock->itf.props.address.un.family != prx_address_family_proxy,
            "Bad address family");

        // Setup async operation structure for open operation
        sock->open_op.context = op_context;
        sock->open_op.addr_len = sizeof(sock->open_op.addr_buf);
        result = pal_os_from_prx_socket_address(&sock->itf.props.address,
            (struct sockaddr*)sock->open_op.addr_buf, &sock->open_op.addr_len);
        if (result != er_ok)
            break;

        // Update address family in properties
        sock->itf.props.family = sock->itf.props.address.un.family;

        // Begin open
        result = pal_socket_open_begin(sock);
        if (result == er_waiting)
            return er_ok; // Wait for callback
    } 
    while (0);

    // Complete open 
    pal_socket_open_complete(sock, result, op_context);
    return result;
}

//
// Cancel async operation
//
static void pal_socket_async_cancel(
    pal_socket_async_t* async_io
)
{
    DWORD error;

    // This will dead end into close complete
    async_io->begin = pal_socket_async_close_begin;

    // Cancel any pending io
    if (!CancelIoEx((HANDLE)async_io->sock->sock_fd, &async_io->ov))
    {
        error = WSAGetLastError();
        if (error == ERROR_NOT_FOUND)
        {
           // async_io->pending = 0;
        }
    }
}

//
// Open a new socket based on properties passed during create
//
int32_t pal_socket_open(
    pal_socket_t *sock,
    void* op_context
)
{
    if (!sock)
        return er_fault;

    dbg_assert(!sock->prx_ai_cur && !sock->prx_ai_count && !sock->prx_ai,
        "Should not have an address list");
    dbg_assert(sock->sock_fd == INVALID_SOCKET, "Socket open");

    if (sock->itf.props.address.un.family == prx_address_family_proxy)
        return pal_socket_open_by_name_begin(sock, op_context);

    return pal_socket_open_by_addr_begin(sock, op_context);
}

//
// Enables send operation loop 
//
int32_t pal_socket_can_send(
    pal_socket_t* sock,
    bool ready
)
{
    if (!sock)
        return er_fault;
    if (sock->sock_fd == INVALID_SOCKET)
        return er_closed;
    if (ready)
        pal_socket_async_begin(&sock->send_op);
    return er_ok;
}

//
// Enables receive operation loop 
//
int32_t pal_socket_can_recv(
    pal_socket_t* sock,
    bool ready
)
{
    if (!sock)
        return er_fault;
    if (sock->sock_fd == INVALID_SOCKET)
        return er_closed;
    if (ready)
        pal_socket_async_begin(&sock->recv_op);
    return er_ok;
}

//
// Create a new sock to track
//
int32_t pal_socket_create(
    pal_socket_client_itf_t* itf,
    pal_socket_t** created
)
{
    pal_socket_t* sock;
    if (!itf || !created || !itf->cb)
        return er_fault;

    sock = mem_zalloc_type(pal_socket_t);
    if (!sock)
        return er_out_of_memory;

    sock->log = log_get("socket");
    sock->sock_fd = INVALID_SOCKET;

    memcpy(&sock->itf, itf, sizeof(pal_socket_client_itf_t));

    // Set function pointers based on type of socket
    sock->send_op.sock =
        sock;
    sock->recv_op.sock =
        sock;
    sock->open_op.sock =
        sock;
    sock->open_op.begin =
        pal_socket_async_no_op;
    sock->open_op.complete =
        pal_socket_async_connect_complete;

    if (sock->itf.props.sock_type == prx_socket_type_dgram ||
        sock->itf.props.sock_type == prx_socket_type_raw)
    {
        // Non connection oriented sockets recvfrom and sendto..
        sock->send_op.begin =
            pal_socket_async_sendto_begin;
        sock->send_op.complete =
            pal_socket_async_sendto_complete;
        sock->recv_op.begin =
            pal_socket_async_recvfrom_begin;
        sock->recv_op.complete =
            pal_socket_async_recvfrom_complete;
    }
    else if (sock->itf.props.flags & socket_flag_passive)
    {
        // Listen socket, can only recv new sockets
        sock->send_op.begin =
            pal_socket_async_no_op;
        sock->send_op.complete =
            NULL;
        sock->recv_op.begin =
            pal_socket_async_accept_begin;
        sock->recv_op.complete =
            pal_socket_async_accept_complete;
    }
    else
    {
        // Stream socket can send and receive - no address
        sock->send_op.begin =
            pal_socket_async_send_begin;
        sock->send_op.complete =
            pal_socket_async_send_complete;
        sock->recv_op.begin =
            pal_socket_async_recv_begin;
        sock->recv_op.complete =
            pal_socket_async_recv_complete;
    }

    *created = sock;
    return er_ok;
}

// 
// Close and disconnect socket
//
void pal_socket_close(
    pal_socket_t *sock,
    void* op_context

)
{
    if (!sock)
        return;

    // Close socket and cancel io in progress
    sock->close_context = op_context ? op_context : (void*)-1;

    pal_socket_async_cancel(&sock->open_op);
    pal_socket_async_cancel(&sock->send_op);
    pal_socket_async_cancel(&sock->recv_op);

    pal_socket_close_complete(sock);
}

//
// Get socket option
//
int32_t pal_socket_getsockopt(
    pal_socket_t* sock,
    prx_socket_option_t socket_option,
    uint64_t* value
)
{
    int32_t result;
    int32_t opt_lvl, opt_name;
    socklen_t opt_len;
    int32_t opt_val;
    u_long avail;

    if (!value)
        return er_fault;

    if (socket_option == prx_so_shutdown)
        return er_not_supported;

    if (socket_option == prx_so_available)
    {
        result = ioctlsocket(sock->sock_fd, FIONREAD, &avail);
        if (result == SOCKET_ERROR)
            return pal_os_last_net_error_as_prx_error();
        *value = avail;
        return er_ok;
    }

    if (socket_option == prx_so_linger)
    {
        struct linger opt;
        opt_len = sizeof(opt);
        result = getsockopt(sock->sock_fd, SOL_SOCKET, SO_LINGER, 
            (char*)&opt, &opt_len);
        if (result != 0)
            return pal_os_last_net_error_as_prx_error();
        *value = opt.l_onoff ? opt.l_linger : 0;
        return er_ok;
    }

    result = pal_os_from_prx_socket_option(socket_option, &opt_lvl, &opt_name);
    if (result != er_ok)
        return result;

    opt_len = sizeof(int32_t);
    result = getsockopt(sock->sock_fd, opt_lvl, opt_name, 
        (char*)&opt_val, &opt_len);
    if (result != 0)
        return pal_os_last_net_error_as_prx_error();

    dbg_assert(opt_len <= (socklen_t)sizeof(int32_t), "invalid len returned");
    if (socket_option == prx_so_error)
        *value = pal_os_to_prx_net_error(opt_val);
    else
        *value = opt_val;
    return er_ok;
}

//
// Set socket option
//
int32_t pal_socket_setsockopt(
    pal_socket_t* sock,
    prx_socket_option_t socket_option,
    uint64_t value
)
{
    int32_t result;
    int32_t opt_lvl, opt_name;
    int32_t opt_val;

    /**/ if (socket_option == prx_so_available)
        return er_not_supported;
    else if (socket_option == prx_so_shutdown)
    {
        if (value != prx_shutdown_op_read)
            sock->send_op.begin = pal_socket_async_no_op;
        if (value != prx_shutdown_op_write)
            sock->recv_op.begin = pal_socket_async_no_op;

        result = pal_os_from_prx_shutdown_op((prx_shutdown_op_t)value, &opt_val);
        if (result != er_ok)
            return result;
        result = shutdown(sock->sock_fd, opt_val);
    }
    else if (socket_option == prx_so_linger)
    {
        struct linger opt;
        opt.l_onoff = !!value;
        opt.l_linger = (unsigned short)value;
        result = setsockopt(sock->sock_fd, SOL_SOCKET, SO_LINGER,
            (char*)&opt, sizeof(opt));
    }
    else if (socket_option == prx_so_nonblocking)
        return er_ok;
    else if (socket_option == prx_so_acceptconn)
        return er_not_supported;
    else
    {
        opt_val = (int32_t)value;
        result = pal_os_from_prx_socket_option(socket_option, &opt_lvl, &opt_name);
        if (result != er_ok)
            return result;

        result = setsockopt(sock->sock_fd, opt_lvl, opt_name, 
            (const char*)&opt_val, (socklen_t)sizeof(opt_val));
    }
    return result == 0 ? er_ok : pal_os_last_net_error_as_prx_error();
}

//
// Get peer address
//
int32_t pal_socket_getpeername(
    pal_socket_t* sock,
    prx_socket_address_t* socket_address
)
{
    if (!sock || !socket_address)
        return er_fault;
    memcpy(socket_address, &sock->peer, sizeof(prx_socket_address_t));
    return er_ok;
}

//
// Get local address
//
int32_t pal_socket_getsockname(
    pal_socket_t* sock,
    prx_socket_address_t* socket_address
)
{
    if (!sock || !socket_address)
        return er_fault;
    memcpy(socket_address, &sock->local, sizeof(prx_socket_address_t));
    return er_ok;
}

//
// Get socket properties
//
int32_t pal_socket_get_properties(
    pal_socket_t *sock,
    prx_socket_properties_t* props
)
{
    if (!sock || !props)
        return er_fault;
    memcpy(props, &sock->itf.props, sizeof(prx_socket_properties_t));
    return er_ok;
}

//
// Leave multicast group
//
int32_t pal_socket_leave_multicast_group(
    pal_socket_t* sock,
    prx_multicast_option_t* option
)
{
    int32_t result;
    struct ipv6_mreq opt6;
    struct ip_mreq opt;

    if (!option)
        return er_fault;

    switch (option->family)
    {
    case prx_address_family_inet:
        opt.imr_multiaddr.s_addr = option->address.in4.un.addr;
        opt.imr_interface.s_addr = option->interface_index;
        result = setsockopt(
            sock->sock_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*)&opt, sizeof(opt));
        break;
    case prx_address_family_inet6:
        memcpy(opt6.ipv6mr_multiaddr.s6_addr, option->address.in6.un.u8,
            sizeof(option->address.in6.un.u8));
        opt6.ipv6mr_interface = option->interface_index;
        result = setsockopt(
            sock->sock_fd, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, (char*)&opt6, sizeof(opt6));
        break;
    default:
        return er_not_supported;
    }
    return result == 0 ? er_ok : pal_os_last_net_error_as_prx_error();
}

//
// Join multicast group
//
int32_t pal_socket_join_multicast_group(
    pal_socket_t* sock,
    prx_multicast_option_t* option
)
{
    int32_t result;
    struct ipv6_mreq opt6;
    struct ip_mreq opt;

    if (!option)
        return er_fault;

    switch (option->family)
    {
    case prx_address_family_inet:
        opt.imr_multiaddr.s_addr = option->address.in4.un.addr;
        opt.imr_interface.s_addr = option->interface_index;
        result = setsockopt(
            sock->sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&opt, sizeof(opt));
        break;
    case prx_address_family_inet6:
        memcpy(opt6.ipv6mr_multiaddr.s6_addr, option->address.in6.un.u8,
            sizeof(option->address.in6.un.u8));
        opt6.ipv6mr_interface = option->interface_index;
        result = setsockopt(
            sock->sock_fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char*)&opt6, sizeof(opt6));
        break;
    default:
        return er_not_supported;
    }
    return result == 0 ? er_ok : pal_os_last_net_error_as_prx_error();
}

//
// Free the socket
//
void pal_socket_free(
    pal_socket_t *sock
)
{
    if (!sock)
        return;

    dbg_assert(sock->sock_fd == INVALID_SOCKET, "socket still open");
    mem_free_type(pal_socket_t, sock);
}

//
// Initialize the winsock layer and retrieve function pointers
//
int32_t pal_socket_init(
    void
)
{
    int error;
    SOCKET s;
    DWORD cb;
    WSADATA wsd;

    GUID guid_connectex = WSAID_CONNECTEX;
    GUID guid_acceptex = WSAID_ACCEPTEX;
    GUID guid_getacceptexsockaddrs = WSAID_GETACCEPTEXSOCKADDRS;

    error = WSAStartup(MAKEWORD(2, 2), &wsd);
    if (error != 0)
        return pal_socket_from_os_error(error);
    do
    {
        // Retrieve winsock function pointers
        s = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
        if (s == INVALID_SOCKET)
        {
            error = SOCKET_ERROR;
            break;
        }

        error = WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER,
            &guid_connectex, sizeof(guid_connectex),
            &_ConnectEx, sizeof(_ConnectEx), &cb, NULL, NULL);
        if (error != 0)
            break;
        error = WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER,
            &guid_acceptex, sizeof(guid_acceptex),
            &_AcceptEx, sizeof(_AcceptEx), &cb, NULL, NULL);
        if (error != 0)
            break;
        error = WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER,
            &guid_getacceptexsockaddrs, sizeof(guid_getacceptexsockaddrs),
            &_GetAcceptExSockAddrs, sizeof(_GetAcceptExSockAddrs),
            &cb, NULL, NULL);
        if (error != 0)
            break;

    } while (0);

    if (s != INVALID_SOCKET)
        closesocket(s);
    if (error != 0)
    {
        log_error(NULL, "Couldn't get WSA function pointers.");
        pal_socket_deinit();
        return pal_os_last_net_error_as_prx_error();
    }
    return er_ok;
}

//
// Deinit socket layer
//
void pal_socket_deinit(
    void
)
{
    int error;
    error = WSACleanup();
    if (!error)
        return;
    // Logs os error as side effect 
    (void)pal_socket_from_os_error(error);
}