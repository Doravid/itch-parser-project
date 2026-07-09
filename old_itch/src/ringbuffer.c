#include "ringbuffer.h"

void init_ringbuffer(ringbuffer_t *rb)
{
    atomic_init(&rb->head, 0);
    atomic_init(&rb->tail, 0);
}

bool push_job(ringbuffer_t *rb, const itch_job_t *job)
{
    size_t current_tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
    size_t next_tail = (current_tail + 1) % RING_BUFFER_SIZE;

    if (next_tail == atomic_load_explicit(&rb->head, memory_order_acquire))
    {
        return false;
    }

    rb->jobs[current_tail] = *job;
    atomic_store_explicit(&rb->tail, next_tail, memory_order_release);
    return true;
}

bool pop_job(ringbuffer_t *rb, itch_job_t *job)
{
    size_t current_head = atomic_load_explicit(&rb->head, memory_order_relaxed);

    if (current_head == atomic_load_explicit(&rb->tail, memory_order_acquire))
    {
        return false;
    }

    *job = rb->jobs[current_head];
    size_t next_head = (current_head + 1) % RING_BUFFER_SIZE;
    atomic_store_explicit(&rb->head, next_head, memory_order_release);
    return true;
}