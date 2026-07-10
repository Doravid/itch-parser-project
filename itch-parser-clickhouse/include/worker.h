#pragma once
#include <pthread.h>
#include <chdb.h>
#include <stdatomic.h>
#include "ringbuffer.h"

typedef struct
{
    ringbuffer_t *queue;
    chdb_connection *shared_db;
    atomic_bool *producer_done;
} worker_args_t;

void *consumer_thread(void *arg);