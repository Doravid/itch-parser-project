
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <emmintrin.h>
#include "parser.h"
#include "ringbuffer.h"
#include "worker.h"
#include "db.h"
#include "filter.h"

#define NUM_WORKERS 8

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s csv <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    size_t size;
    uint8_t *mapped_file = map_file_into_memory(argv[2], &size);
    field_filter_t filter;
    filter_init_from_csv(argv[1], &filter);
    duckdb_database db;
    if (db_global_init(&db, "./duck.db", &filter) != 0)
    {
        fprintf(stderr, "Global DB Init Failed.\n");
        return EXIT_FAILURE;
    }

    ringbuffer_t queues[NUM_WORKERS];
    for (int i = 0; i < NUM_WORKERS; i++)
    {
        init_ringbuffer(&queues[i]);
    }

    atomic_bool producer_done;
    atomic_init(&producer_done, false);

    worker_args_t w_args[NUM_WORKERS];
    pthread_t workers[NUM_WORKERS];

    for (int i = 0; i < NUM_WORKERS; i++)
    {
        w_args[i].queue = &queues[i];
        w_args[i].shared_db = db;
        w_args[i].producer_done = &producer_done;
        w_args[i].filter = &filter;
        pthread_create(&workers[i], NULL, consumer_thread, &w_args[i]);
    }

    const uint8_t *end = mapped_file + size;
    const uint8_t *ptr = mapped_file;
    const uint8_t *block_start = ptr;
    size_t msg_count = 0;
    int worker_idx = 0;

    while (ptr + 2 <= end)
    {
        uint16_t msg_len = (uint16_t)((ptr[0] << 8) | ptr[1]);
        ptr += 2 + msg_len;
        msg_count++;

        if (msg_count >= 200000 || ptr >= end)
        {
            itch_job_t job = {.start = block_start, .end = ptr};

            while (!push_job(&queues[worker_idx], &job))
            {
                _mm_pause();
            }

            block_start = ptr;
            msg_count = 0;
            worker_idx = (worker_idx + 1) % NUM_WORKERS;
        }
    }

    atomic_store_explicit(&producer_done, true, memory_order_release);

    for (int i = 0; i < NUM_WORKERS; i++)
    {
        pthread_join(workers[i], NULL);
    }

    db_global_finalize(db, &filter);
    unmap_file(mapped_file, size);
    return EXIT_SUCCESS;
}