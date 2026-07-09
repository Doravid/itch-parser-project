
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "db.h"
#include "filter.h"
#include "parser.h"

static inline void db_append_u64(duckdb_appender app, uint64_t val)
{
    if (duckdb_append_uint64(app, val) == DuckDBError)
        exit(EXIT_FAILURE);
}

static inline void db_append_u32(duckdb_appender app, uint32_t val)
{
    if (duckdb_append_uint32(app, val) == DuckDBError)
        exit(EXIT_FAILURE);
}

static inline void db_append_u8(duckdb_appender app, uint8_t val)
{
    if (duckdb_append_uint8(app, val) == DuckDBError)
        exit(EXIT_FAILURE);
}

static inline void db_append_blob(duckdb_appender app, const void *data, size_t len)
{
    if (duckdb_append_blob(app, data, len) == DuckDBError)
        exit(EXIT_FAILURE);
}

int db_global_init(duckdb_database *db, const char *db_path, const struct field_filter *filter)
{
    if (duckdb_open(db_path, db) == DuckDBError)
        return -1;

    duckdb_connection conn;
    if (duckdb_connect(*db, &conn) == DuckDBError)
        return -1;

    char ddl[8192];

    if (filter->market.enabled)
    {
        filter_build_create_table_sql(&filter->market, "itch_market", ddl, sizeof(ddl));
        duckdb_query(conn, ddl, NULL);
    }
    if (filter->admin.enabled)
    {
        filter_build_create_table_sql(&filter->admin, "itch_admin", ddl, sizeof(ddl));
        duckdb_query(conn, ddl, NULL);
    }
    if (filter->extended.enabled)
    {
        filter_build_create_table_sql(&filter->extended, "itch_extended", ddl, sizeof(ddl));
        duckdb_query(conn, ddl, NULL);
    }

    duckdb_query(conn, "PRAGMA preserve_insertion_order=false;", NULL);

    duckdb_disconnect(&conn);
    return 0;
}

int db_worker_init(db_ctx_t *ctx, duckdb_database db, const struct field_filter *filter)
{
    ctx->filter = filter;
    ctx->appender_market = NULL;
    ctx->appender_admin = NULL;
    ctx->appender_extended = NULL;

    if (duckdb_connect(db, &ctx->conn) == DuckDBError)
        return -1;

    if (filter->market.enabled &&
        duckdb_appender_create(ctx->conn, NULL, "itch_market", &ctx->appender_market) == DuckDBError)
        return -1;
    if (filter->admin.enabled &&
        duckdb_appender_create(ctx->conn, NULL, "itch_admin", &ctx->appender_admin) == DuckDBError)
        return -1;
    if (filter->extended.enabled &&
        duckdb_appender_create(ctx->conn, NULL, "itch_extended", &ctx->appender_extended) == DuckDBError)
        return -1;

    return 0;
}

/**
 * @brief Appends the selected columns of one record to one appender.
 * @param app The appender to write to.
 * @param msg Pointer to the record struct (db_admin_record / db_market_record / db_extended_record).
 * @param tf The table_filter_t describing which fields (and in what order) to write.
 * @return 0 on success, -1 on failure.
 */
static int db_append_generic(duckdb_appender app, const void *msg, const table_filter_t *tf)
{
    for (size_t i = 0; i < tf->count; i++)
    {
        const field_desc_t *f = tf->fields[i];
        const uint8_t *p = (const uint8_t *)msg + f->offset;

        switch (f->ctype)
        {
        case FIELD_U8:
            db_append_u8(app, *(const uint8_t *)p);
            break;
        case FIELD_U32:
            db_append_u32(app, *(const uint32_t *)p);
            break;
        case FIELD_U64:
            db_append_u64(app, *(const uint64_t *)p);
            break;
        case FIELD_BLOB:
            db_append_blob(app, p, f->blob_len);
            break;
        }
    }
    return duckdb_appender_end_row(app) == DuckDBError ? -1 : 0;
}

int db_append_admin(db_ctx_t *state, const struct db_admin_record *msg)
{
    if (!state->filter->admin.enabled)
        return 0;
    return db_append_generic(state->appender_admin, msg, &state->filter->admin);
}

int db_append_market(db_ctx_t *state, const struct db_market_record *msg)
{
    if (!state->filter->market.enabled)
        return 0;
    return db_append_generic(state->appender_market, msg, &state->filter->market);
}

int db_append_extended(db_ctx_t *state, const struct db_extended_record *msg)
{
    if (!state->filter->extended.enabled)
        return 0;
    return db_append_generic(state->appender_extended, msg, &state->filter->extended);
}

void db_worker_flush(db_ctx_t *ctx)
{
    if (ctx->appender_market && duckdb_appender_flush(ctx->appender_market) == DuckDBError)
        exit(EXIT_FAILURE);
    if (ctx->appender_admin && duckdb_appender_flush(ctx->appender_admin) == DuckDBError)
        exit(EXIT_FAILURE);
    if (ctx->appender_extended && duckdb_appender_flush(ctx->appender_extended) == DuckDBError)
        exit(EXIT_FAILURE);
}

void db_worker_close(db_ctx_t *ctx)
{
    if (ctx->appender_market)
        duckdb_appender_destroy(&ctx->appender_market);
    if (ctx->appender_admin)
        duckdb_appender_destroy(&ctx->appender_admin);
    if (ctx->appender_extended)
        duckdb_appender_destroy(&ctx->appender_extended);
    duckdb_disconnect(&ctx->conn);
}

void db_global_finalize(duckdb_database db, const struct field_filter *filter)
{
    duckdb_connection conn;
    if (duckdb_connect(db, &conn) == DuckDBError)
        return;

    const table_filter_t *tables[3] = {&filter->market, &filter->admin, &filter->extended};
    const char *table_names[3] = {"itch_market", "itch_admin", "itch_extended"};

    char selects[3][8192];
    int enabled_idx[3];
    int enabled_count = 0;

    for (int i = 0; i < 3; i++)
    {
        if (tables[i]->enabled)
        {
            filter_build_select_sql(tables[i], table_names[i], selects[i], sizeof(selects[i]));
            enabled_idx[enabled_count++] = i;
        }
    }

    if (enabled_count > 0)
    {
        char view_sql[32768];
        int n = snprintf(view_sql, sizeof(view_sql), "CREATE VIEW itch_unified AS ");
        for (int i = 0; i < enabled_count; i++)
        {
            n += snprintf(view_sql + n, (size_t)n < sizeof(view_sql) ? sizeof(view_sql) - (size_t)n : 0,
                          "%s%s", (i == 0 ? "" : " UNION ALL BY NAME "), selects[enabled_idx[i]]);
        }
        duckdb_query(conn, view_sql, NULL);
    }

    duckdb_disconnect(&conn);
    duckdb_close(&db);
}