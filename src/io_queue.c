// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "util_mem.h"
#include "io_queue.h"
#include "prx_buffer.h"
#include "pal_mt.h"

//
// A tristate - ready, in progress, completed - queue of buffers for IO
//
typedef struct io_queue
{
    lock_t queue_lock;
    prx_buffer_factory_t* factory;      // Factory used to create buffer
    DLIST_ENTRY ready;   // queue_buffers that are ready for processing
    DLIST_ENTRY inprogress;       // queue_buffers that are in progress
    DLIST_ENTRY done;           // queue_buffers that are in done state
}
io_queue_t;

//
// Calls callback with abort
//
static void io_queue_buffer_abort_callback(
    io_queue_buffer_t* queue_buffer
)
{
    void(*cb)(void*, int32_t);
    void* ctx;

    cb = queue_buffer->cb_ptr;
    queue_buffer->cb_ptr = NULL;
    ctx = queue_buffer->ctx;
    queue_buffer->ctx = NULL;

    if (cb) cb(ctx, er_aborted);
}

//
// Free queue_buffer
//
static void io_queue_release_buffer_no_lock(
    io_queue_t* queue,
    io_queue_buffer_t* queue_buffer
)
{
    if (!queue_buffer)
        return;
    DList_RemoveEntryList(&queue_buffer->qlink);
    DList_InitializeListHead(&queue_buffer->qlink);
    io_queue_buffer_abort_callback(queue_buffer);
    prx_buffer_release(queue->factory, queue_buffer);
}

//
// move the buffer into the specified state list
//
static void io_queue_state_push(
    io_queue_t* queue,
    PDLIST_ENTRY list,
    io_queue_buffer_t* queue_buffer
)
{
    dbg_assert_ptr(queue_buffer);
    dbg_assert_ptr(queue);
    dbg_assert_ptr(list);
    lock_enter(queue->queue_lock);
    DList_RemoveEntryList(&queue_buffer->qlink);
    DList_InsertTailList(list, &queue_buffer->qlink);
    lock_exit(queue->queue_lock);
}

//
// Whether ready queue_buffers are present
//
static bool io_queue_state_peek(
    io_queue_t* queue,
    PDLIST_ENTRY list
)
{
    bool not_empty;
    lock_enter(queue->queue_lock);
    not_empty = !DList_IsListEmpty(list);
    lock_exit(queue->queue_lock);
    return not_empty;
}

//
// retrieve the first queue_buffer that is in state list or null
//
static io_queue_buffer_t* io_queue_state_pop(
    io_queue_t* queue,
    PDLIST_ENTRY list
)
{
    io_queue_buffer_t* queue_buffer = NULL;
    if (queue)
    {
        lock_enter(queue->queue_lock);
        if (!DList_IsListEmpty(list))
        {
            queue_buffer = containingRecord(
                DList_RemoveHeadList(list), io_queue_buffer_t, qlink);
            DList_InitializeListHead(&queue_buffer->qlink);
        }
        lock_exit(queue->queue_lock);
    }
    return queue_buffer;
}

//
// Free all buffers in the given state
//
static void io_queue_state_release_no_lock(
    io_queue_t* queue,
    PDLIST_ENTRY list
)
{
    while (!DList_IsListEmpty(list))
        io_queue_release_buffer_no_lock(queue, containingRecord(
            DList_RemoveHeadList(list), io_queue_buffer_t, qlink));
    DList_InitializeListHead(list);
}

//
// Remove and free all queue_buffers 
//
void io_queue_release_all_buffers(
    io_queue_t* queue
)
{
    dbg_assert_ptr(queue);
    lock_enter(queue->queue_lock);
    io_queue_state_release_no_lock(queue, &queue->done);
    io_queue_state_release_no_lock(queue, &queue->inprogress);
    io_queue_state_release_no_lock(queue, &queue->ready);
    lock_exit(queue->queue_lock);
}

//
// Abort all buffers in the given state
//
static void io_queue_state_abort_no_lock(
    PDLIST_ENTRY list
)
{
    for (PDLIST_ENTRY p = list->Flink; p != list; p = p->Flink)
        io_queue_buffer_abort_callback(containingRecord(p, io_queue_buffer_t, qlink));
}

//
// Creates a new queue_buffer of given size and with given content
//
int32_t io_queue_create_buffer(
    io_queue_t* queue,
    const void* payload,
    size_t length,
    io_queue_buffer_t** created
)
{
    int32_t result = er_out_of_memory;
    io_queue_buffer_t* queue_buffer;

    if (!created || length < 0 || !queue)
        return er_fault;

    queue_buffer = (io_queue_buffer_t*)prx_buffer_new(
        queue->factory, length + sizeof(io_queue_buffer_t));
    if (!queue_buffer)
        return er_out_of_memory;
    do
    {
        memset(queue_buffer, 0, sizeof(io_queue_buffer_t));
        DList_InitializeListHead(&queue_buffer->qlink);

        queue_buffer->queue = queue;   // Factory used to create queue_buffer
        queue_buffer->code = er_ok;
        queue_buffer->length = length;

        if (payload && length > 0)
        {
            result = io_queue_buffer_write(queue_buffer, payload, length);
            if (result != er_ok)
                break;
        }

        *created = queue_buffer;
        return er_ok;
    } while (0);

    io_queue_release_buffer_no_lock(queue, queue_buffer);
    return result;
}

//
// Get the raw buffer pointer from handle
//
uint8_t* io_queue_buffer_to_ptr(
    io_queue_buffer_t* queue_buffer
)
{
     return ((uint8_t*)queue_buffer) + sizeof(io_queue_buffer_t);
}

//
// Get the buffer handle from raw pointer
//
io_queue_buffer_t* io_queue_buffer_from_ptr(
    uint8_t* payload
)
{
     return (io_queue_buffer_t*)(payload - sizeof(io_queue_buffer_t));
}

//
// Free queue_buffer and invoke registered callback
//
void io_queue_buffer_release(
    io_queue_buffer_t* queue_buffer
)
{
    io_queue_t* queue;
    if (!queue_buffer || !queue_buffer->queue)
        return;

    queue = queue_buffer->queue;
    queue_buffer->queue = NULL;

    io_queue_buffer_abort_callback(queue_buffer);

    lock_enter(queue->queue_lock);
    io_queue_release_buffer_no_lock(queue, queue_buffer);
    lock_exit(queue->queue_lock);
}

//
// Copy to queue_buffer
//
int32_t io_queue_buffer_write(
    io_queue_buffer_t* queue_buffer,
    const void* buf,
    size_t len
)
{
    size_t write;

    if (!queue_buffer || len < 0)
        return er_fault;
    if (len == 0)
        return er_ok;

    write = min(queue_buffer->length - queue_buffer->write_offset, len);

    memcpy(io_queue_buffer_to_ptr(queue_buffer) + queue_buffer->write_offset, buf, write);
    queue_buffer->write_offset += write;
    return er_ok;
}

//
// Copy from the queue_buffer
//
int32_t io_queue_buffer_read(
    io_queue_buffer_t* queue_buffer,
    void* buf,
    size_t len,
    size_t *read
)
{
    size_t available;

    if (!queue_buffer || !buf || len < 0 || !read)
        return er_fault;
    if (len == 0)
        return er_ok;

    available = queue_buffer->length - queue_buffer->read_offset;
    *read = min(available, len);

    memcpy(buf, io_queue_buffer_to_ptr(queue_buffer) + queue_buffer->read_offset, *read);
    queue_buffer->read_offset += *read;
    return er_ok;
}

//
// move the buffer into "in progress" state
//
void io_queue_buffer_set_inprogress(
    io_queue_buffer_t* queue_buffer
)
{
    if (!queue_buffer || !queue_buffer->queue)
        return;
    io_queue_state_push(queue_buffer->queue,
        &queue_buffer->queue->inprogress, queue_buffer);
}

//
// move the buffer into in "ready" state
//
void io_queue_buffer_set_ready(
    io_queue_buffer_t* queue_buffer
)
{
    if (!queue_buffer || !queue_buffer->queue)
        return;
    io_queue_state_push(queue_buffer->queue,
        &queue_buffer->queue->ready, queue_buffer);
}

//
// move the buffer into in "done" state
//
void io_queue_buffer_set_done(
    io_queue_buffer_t* queue_buffer
)
{
    if (!queue_buffer || !queue_buffer->queue)
        return;
    io_queue_state_push(queue_buffer->queue,
        &queue_buffer->queue->done, queue_buffer);
}

//
// Frees a queue that was allocated 
//
void io_queue_free(
	io_queue_t* queue
	)
{
	if (!queue)
		return;

    if (queue->queue_lock)
    {
        io_queue_release_all_buffers(queue);
        lock_free(queue->queue_lock);
    }
    
    if (queue->factory)
        prx_buffer_factory_free(queue->factory);

    mem_free_type(io_queue_t, queue);
}

// 
// Helper to allocate a queue of payload buffers
//
int32_t io_queue_create(
    const char* name,
    io_queue_t** created
)
{
	io_queue_t* queue;
	int32_t result;

    if (!created)
        return er_fault;

	queue = mem_zalloc_type(io_queue_t);
	if (!queue) 
		return er_out_of_memory;
    do
    {
	    DList_InitializeListHead(&queue->inprogress);
        DList_InitializeListHead(&queue->ready);
        DList_InitializeListHead(&queue->done);

        result = lock_create(&queue->queue_lock);
        if (result != er_ok)
            break;

        result = prx_dynamic_pool_create(name, 0, NULL, &queue->factory);
        if (result != er_ok)
            break;

        *created = queue;
        return er_ok;
    } while (0);

	io_queue_free(queue);
	return result;
}

//
// Whether ready queue_buffers are present
//
bool io_queue_has_ready(
    io_queue_t* queue
)
{
    if (!queue)
        return false;
    return io_queue_state_peek(queue, &queue->ready);
}

//
// retrieve the first queue_buffer that is "ready" from the queue
//
io_queue_buffer_t* io_queue_pop_ready(
	io_queue_t* queue
)
{
    if (!queue)
        return NULL;
    return io_queue_state_pop(queue, &queue->ready);
}

//
// Whether in progress queue_buffers are present
//
bool io_queue_has_inprogress(
    io_queue_t* queue
)
{
    if (!queue)
        return false;
    return io_queue_state_peek(queue, &queue->inprogress);
}

//
// retrieve the first queue_buffer that is in progress
//
io_queue_buffer_t* io_queue_pop_inprogress(
	io_queue_t* queue
	)
{
    return io_queue_state_pop(queue, &queue->inprogress);
}

//
// Whether in completed queue_buffers are present
//
bool io_queue_has_done(
    io_queue_t* queue
)
{
    if (!queue)
        return false;
    return io_queue_state_peek(queue, &queue->done);
}

//
// retrieve the first completed buffer in queue
//
io_queue_buffer_t* io_queue_pop_done(
    io_queue_t* queue
)
{
    return io_queue_state_pop(queue, &queue->done);
}


//
// requeue everything in progress into ready queue
//
void io_queue_rollback(
	io_queue_t* queue
	)
{
    if (!queue)
        return;
    lock_enter(queue->queue_lock);
    DList_AppendTailList(queue->ready.Flink, &queue->inprogress);
    DList_RemoveEntryList(&queue->inprogress);
    DList_InitializeListHead(&queue->inprogress); 
    lock_exit(queue->queue_lock);
}

//
// Detach all contexts from the buffers
//
void io_queue_abort(
    io_queue_t* queue
)
{
    if (!queue)
        return;
    lock_enter(queue->queue_lock);
    io_queue_state_abort_no_lock(&queue->done);
    io_queue_state_abort_no_lock(&queue->inprogress);
    io_queue_state_abort_no_lock(&queue->ready);
    lock_exit(queue->queue_lock);
}
