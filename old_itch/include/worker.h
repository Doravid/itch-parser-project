#pragma once
#include <pthread.h>
#include <duckdb.h>
#include "ringbuffer.h"

struct field_filter;

typedef struct
{
    ringbuffer_t *queue;
    duckdb_database shared_db;
    atomic_bool *producer_done;
    const struct field_filter *filter;
} worker_args_t;

void *consumer_thread(void *arg);