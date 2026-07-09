#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "db.h"

/* How to read a field out of its owning struct (db_admin_record /
 * db_market_record / db_extended_record) and how to describe it in SQL. */
typedef enum
{
    FIELD_U8,
    FIELD_U32,
    FIELD_U64,
    FIELD_BLOB,
} field_ctype_t;

typedef struct
{
    const char *name;     /* user-facing name, matched against --fields */
    const char *sql_type; /* DuckDB column type */
    field_ctype_t ctype;
    size_t offset;         /* offsetof() into the owning record struct */
    size_t blob_len;       /* only meaningful when ctype == FIELD_BLOB */
    const char *msg_types; /* ASCII msg_type chars that populate this field */
} field_desc_t;

/* Master, canonical-order field lists. Column order in CREATE TABLE and in
 * the appender both derive from this order, so they can never disagree. */
extern const field_desc_t ADMIN_FIELD_DEFS[];
extern const size_t ADMIN_FIELD_COUNT;
extern const field_desc_t MARKET_FIELD_DEFS[];
extern const size_t MARKET_FIELD_COUNT;
extern const field_desc_t EXTENDED_FIELD_DEFS[];
extern const size_t EXTENDED_FIELD_COUNT;

extern const char *const ADMIN_MSG_TYPES;
extern const char *const MARKET_MSG_TYPES;
extern const char *const EXTENDED_MSG_TYPES;

#define TABLE_FILTER_MAX_FIELDS 64

typedef struct
{
    const field_desc_t *fields[TABLE_FILTER_MAX_FIELDS];
    size_t count;
    bool enabled;
} table_filter_t;

struct field_filter
{
    table_filter_t admin;
    table_filter_t market;
    table_filter_t extended;
    bool should_process[256];
};
typedef struct field_filter field_filter_t;

void filter_init_all(field_filter_t *out);

void filter_init_from_csv(const char *csv, field_filter_t *out);

void filter_build_create_table_sql(const table_filter_t *tf, const char *table_name,
                                   char *buf, size_t bufsize);
void filter_build_select_sql(const table_filter_t *tf, const char *table_name,
                             char *buf, size_t bufsize);