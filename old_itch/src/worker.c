#include "worker.h"
#include "parser.h"
#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <emmintrin.h>

void *consumer_thread(void *arg)
{
    worker_args_t *args = (worker_args_t *)arg;
    db_ctx_t ctx;
    if (db_worker_init(&ctx, args->shared_db, args->filter) != 0)
    {
        fprintf(stderr, "Worker Failed to Initialize DuckDB.\n");
        exit(EXIT_FAILURE);
    }
    itch_job_t job;
    while (true)
    {
        if (pop_job(args->queue, &job))
        {
            parse_itch_stream(job.start, job.end - job.start, &ctx);
            db_worker_flush(&ctx);
        }
        else
        {
            if (atomic_load_explicit(args->producer_done, memory_order_acquire))
            {
                if (!pop_job(args->queue, &job))
                {
                    break;
                }
                parse_itch_stream(job.start, job.end - job.start, &ctx);
                db_worker_flush(&ctx);
            }
            _mm_pause();
        }
    }
    db_worker_close(&ctx);
    return NULL;
}