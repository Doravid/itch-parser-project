#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "db.h"
#include "parser.h"

int main(int argc, char **argv)
{
    if (argc != 2)
        return EXIT_FAILURE;

    size_t size;
    uint8_t *mapped_file = map_file(argv[1], &size);

    db_state_t state;
    duckdb_database db;
    if (db_init(&db, "./duck.db", &state) != 0)
    {
        unmap_file(mapped_file, size);
        return EXIT_FAILURE;
    }

    uint64_t msg_count = parse_stream(mapped_file, size, &state);

    printf("msg_count: %lu\n", (unsigned long)msg_count);

    db_close(db, &state);
    unmap_file(mapped_file, size);

    return EXIT_SUCCESS;
}