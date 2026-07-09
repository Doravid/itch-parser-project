#pragma once
#include <duckdb.h>

struct field_filter;

typedef struct
{
    duckdb_database db;
    duckdb_connection conn;
    duckdb_appender appender_market;
    duckdb_appender appender_admin;
    duckdb_appender appender_extended;
    const struct field_filter *filter;
} db_ctx_t;