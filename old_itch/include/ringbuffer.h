#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

#define RING_BUFFER_SIZE 256

typedef struct
{
    const uint8_t *start;
    const uint8_t *end;
} itch_job_t;

typedef struct
{
    itch_job_t jobs[RING_BUFFER_SIZE];
    atomic_size_t head;
    atomic_size_t tail;
} ringbuffer_t;

void init_ringbuffer(ringbuffer_t *rb);
bool push_job(ringbuffer_t *rb, const itch_job_t *job);
bool pop_job(ringbuffer_t *rb, itch_job_t *job);