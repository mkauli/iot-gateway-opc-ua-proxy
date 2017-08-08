// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef _pal_ev_h_
#define _pal_ev_h_

#include "common.h"
#include "pal_sk.h"

//
// Socket event types
//
typedef enum pal_event_type
{
    pal_event_type_unknown = 0,
    pal_event_type_read,
    pal_event_type_write,
    pal_event_type_close,
    pal_event_type_error,
    pal_event_type_destroy
}
pal_event_type_t;

//
// Callback to notify about events
//
typedef int32_t (*pal_event_port_handler_t)(
    void* context,
    pal_event_type_t event_type,
    int32_t error_code
    );

//
// Callback to notify about timeouts
//
typedef int32_t (*pal_timeout_handler_t)(
    void* context,
    bool no_events
    );

//
// Create and start event port
//
decl_public_3(int32_t, pal_event_port_create,
    pal_timeout_handler_t, timeout_handler,
    void*, context,
    uintptr_t*, port
);

//
// Register event callback
//
decl_public_5(int32_t, pal_event_port_register,
    uintptr_t, port,
    intptr_t, sock,
    pal_event_port_handler_t, cb,
    void*, context,
    uintptr_t*, event_handle
);

//
// Register interest in a certain type of event
//
decl_public_2(int32_t, pal_event_select,
    uintptr_t, event_handle,
    pal_event_type_t, event_type
);

//
// Clear interest in event
//
decl_public_2(int32_t, pal_event_clear,
    uintptr_t, event_handle,
    pal_event_type_t, event_type
);

//
// Closes the event
//
decl_public_2(void, pal_event_close,
    uintptr_t, event_handle,
    bool, close_fd
);

//
// Stop event port
//
decl_public_1(void, pal_event_port_stop,
    uintptr_t, port
);

//
// Close event port
//
decl_public_1(void, pal_event_port_close,
    uintptr_t, port
);

#endif // _pal_ev_h_
