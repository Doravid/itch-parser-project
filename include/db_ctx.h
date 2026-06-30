#pragma once
#include <duckdb.h>
typedef struct
{
    duckdb_database db;
    duckdb_connection conn;
    duckdb_appender appender_market;
    duckdb_appender appender_admin;
} db_ctx_t;