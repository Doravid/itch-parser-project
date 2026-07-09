#include "filter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *const ADMIN_MSG_TYPES = "SRHYLVWKJhNO";
const char *const MARKET_MSG_TYPES = "AFXD";
const char *const EXTENDED_MSG_TYPES = "IQBECPU";

/* msg_types strings below reflect exactly which branch in parse_itch_stream
 * sets each field. If parser.c changes what a message type populates,
 * update the corresponding line here. */

const field_desc_t ADMIN_FIELD_DEFS[] = {
    {"timestamp", "UBIGINT", FIELD_U64, offsetof(struct db_admin_record, timestamp), 0, "SRHYLVWKJhNO"},
    {"msg_type", "UTINYINT", FIELD_U8, offsetof(struct db_admin_record, msg_type), 0, "SRHYLVWKJhNO"},
    {"stock", "BLOB", FIELD_BLOB, offsetof(struct db_admin_record, stock), 8, "RHYLKJhNO"},
    {"reason", "BLOB", FIELD_BLOB, offsetof(struct db_admin_record, reason), 4, "H"},
    {"issue_sub_type", "BLOB", FIELD_BLOB, offsetof(struct db_admin_record, issue_sub_type), 2, "R"},
    {"level_1", "UBIGINT", FIELD_U64, offsetof(struct db_admin_record, level_1), 0, "V"},
    {"level_2", "UBIGINT", FIELD_U64, offsetof(struct db_admin_record, level_2), 0, "V"},
    {"level_3", "UBIGINT", FIELD_U64, offsetof(struct db_admin_record, level_3), 0, "V"},
    {"near_exec_time", "UBIGINT", FIELD_U64, offsetof(struct db_admin_record, near_exec_time), 0, "O"},
    {"round_lot_size", "UINTEGER", FIELD_U32, offsetof(struct db_admin_record, round_lot_size), 0, "R"},
    {"etp_leverage", "UINTEGER", FIELD_U32, offsetof(struct db_admin_record, etp_leverage), 0, "R"},
    {"ipo_release_time", "UINTEGER", FIELD_U32, offsetof(struct db_admin_record, ipo_release_time), 0, "K"},
    {"ipo_price", "UINTEGER", FIELD_U32, offsetof(struct db_admin_record, ipo_price), 0, "K"},
    {"ref_price", "UINTEGER", FIELD_U32, offsetof(struct db_admin_record, ref_price), 0, "J"},
    {"upper_collar", "UINTEGER", FIELD_U32, offsetof(struct db_admin_record, upper_collar), 0, "JO"},
    {"lower_collar", "UINTEGER", FIELD_U32, offsetof(struct db_admin_record, lower_collar), 0, "JO"},
    {"extension", "UINTEGER", FIELD_U32, offsetof(struct db_admin_record, extension), 0, "J"},
    {"min_allowable_price", "UINTEGER", FIELD_U32, offsetof(struct db_admin_record, min_allowable_price), 0, "O"},
    {"max_allowable_price", "UINTEGER", FIELD_U32, offsetof(struct db_admin_record, max_allowable_price), 0, "O"},
    {"near_exec_price", "UINTEGER", FIELD_U32, offsetof(struct db_admin_record, near_exec_price), 0, "O"},
    {"event_code", "UTINYINT", FIELD_U8, offsetof(struct db_admin_record, event_code), 0, "S"},
    {"market_cat", "UTINYINT", FIELD_U8, offsetof(struct db_admin_record, market_cat), 0, "R"},
    {"fin_status", "UTINYINT", FIELD_U8, offsetof(struct db_admin_record, fin_status), 0, "R"},
    {"round_lots_only", "UTINYINT", FIELD_U8, offsetof(struct db_admin_record, round_lots_only), 0, "R"},
    {"issue_class", "UTINYINT", FIELD_U8, offsetof(struct db_admin_record, issue_class), 0, "R"},
    {"authenticity", "UTINYINT", FIELD_U8, offsetof(struct db_admin_record, authenticity), 0, "R"},
    {"short_sale_thresh", "UTINYINT", FIELD_U8, offsetof(struct db_admin_record, short_sale_thresh), 0, "R"},
    {"ipo_flag", "UTINYINT", FIELD_U8, offsetof(struct db_admin_record, ipo_flag), 0, "R"},
    {"luld_tier", "UTINYINT", FIELD_U8, offsetof(struct db_admin_record, luld_tier), 0, "R"},
    {"etp_flag", "UTINYINT", FIELD_U8, offsetof(struct db_admin_record, etp_flag), 0, "R"},
    {"inverse_indicator", "UTINYINT", FIELD_U8, offsetof(struct db_admin_record, inverse_indicator), 0, "R"},
    {"trading_state", "UTINYINT", FIELD_U8, offsetof(struct db_admin_record, trading_state), 0, "H"},
    {"reg_sho_action", "UTINYINT", FIELD_U8, offsetof(struct db_admin_record, reg_sho_action), 0, "Y"},
    {"primary_mm", "UTINYINT", FIELD_U8, offsetof(struct db_admin_record, primary_mm), 0, "L"},
    {"mm_mode", "UTINYINT", FIELD_U8, offsetof(struct db_admin_record, mm_mode), 0, "L"},
    {"mp_state", "UTINYINT", FIELD_U8, offsetof(struct db_admin_record, mp_state), 0, "L"},
    {"breached_level", "UTINYINT", FIELD_U8, offsetof(struct db_admin_record, breached_level), 0, "W"},
    {"release_qualifier", "UTINYINT", FIELD_U8, offsetof(struct db_admin_record, release_qualifier), 0, "K"},
    {"market_code", "UTINYINT", FIELD_U8, offsetof(struct db_admin_record, market_code), 0, "h"},
    {"halt_action", "UTINYINT", FIELD_U8, offsetof(struct db_admin_record, halt_action), 0, "h"},
    {"interest_flag", "UTINYINT", FIELD_U8, offsetof(struct db_admin_record, interest_flag), 0, "N"},
    {"open_eligibility", "UTINYINT", FIELD_U8, offsetof(struct db_admin_record, open_eligibility), 0, "O"},
};
const size_t ADMIN_FIELD_COUNT = sizeof(ADMIN_FIELD_DEFS) / sizeof(ADMIN_FIELD_DEFS[0]);

const field_desc_t MARKET_FIELD_DEFS[] = {
    {"timestamp", "UBIGINT", FIELD_U64, offsetof(struct db_market_record, timestamp), 0, "AFXD"},
    {"msg_type", "UTINYINT", FIELD_U8, offsetof(struct db_market_record, msg_type), 0, "AFXD"},
    {"stock", "BLOB", FIELD_BLOB, offsetof(struct db_market_record, stock), 8, "AF"},
    {"mpid", "BLOB", FIELD_BLOB, offsetof(struct db_market_record, mpid), 4, "F"},
    {"order_ref", "UBIGINT", FIELD_U64, offsetof(struct db_market_record, order_ref), 0, "AFXD"},
    {"shares", "UINTEGER", FIELD_U32, offsetof(struct db_market_record, shares), 0, "AFX"},
    {"price", "UINTEGER", FIELD_U32, offsetof(struct db_market_record, price), 0, "AF"},
    {"buy_sell", "UTINYINT", FIELD_U8, offsetof(struct db_market_record, buy_sell), 0, "AF"},
};
const size_t MARKET_FIELD_COUNT = sizeof(MARKET_FIELD_DEFS) / sizeof(MARKET_FIELD_DEFS[0]);

const field_desc_t EXTENDED_FIELD_DEFS[] = {
    {"timestamp", "UBIGINT", FIELD_U64, offsetof(struct db_extended_record, timestamp), 0, "IQBECPU"},
    {"msg_type", "UTINYINT", FIELD_U8, offsetof(struct db_extended_record, msg_type), 0, "IQBECPU"},
    {"stock", "BLOB", FIELD_BLOB, offsetof(struct db_extended_record, stock), 8, "QIP"},
    {"match_number", "UBIGINT", FIELD_U64, offsetof(struct db_extended_record, match_number), 0, "BECPQ"},
    {"paired_shares", "UBIGINT", FIELD_U64, offsetof(struct db_extended_record, paired_shares), 0, "IQ"},
    {"imbalance_shares", "UBIGINT", FIELD_U64, offsetof(struct db_extended_record, imbalance_shares), 0, "I"},
    {"far_price", "UINTEGER", FIELD_U32, offsetof(struct db_extended_record, far_price), 0, "I"},
    {"near_price", "UINTEGER", FIELD_U32, offsetof(struct db_extended_record, near_price), 0, "I"},
    {"current_ref_price", "UINTEGER", FIELD_U32, offsetof(struct db_extended_record, current_ref_price), 0, "I"},
    {"price", "UINTEGER", FIELD_U32, offsetof(struct db_extended_record, price), 0, "QCUP"},
    {"cross_type", "UTINYINT", FIELD_U8, offsetof(struct db_extended_record, cross_type), 0, "QI"},
    {"imbalance_dir", "UTINYINT", FIELD_U8, offsetof(struct db_extended_record, imbalance_dir), 0, "I"},
    {"price_var_ind", "UTINYINT", FIELD_U8, offsetof(struct db_extended_record, price_var_ind), 0, "I"},
    {"printable", "UTINYINT", FIELD_U8, offsetof(struct db_extended_record, printable), 0, "C"},
    {"order_ref", "UBIGINT", FIELD_U64, offsetof(struct db_extended_record, order_ref), 0, "ECUP"},
    {"new_order_ref", "UBIGINT", FIELD_U64, offsetof(struct db_extended_record, new_order_ref), 0, "U"},
    {"shares", "UINTEGER", FIELD_U32, offsetof(struct db_extended_record, shares), 0, "ECUP"},
    {"buy_sell", "UTINYINT", FIELD_U8, offsetof(struct db_extended_record, buy_sell), 0, "P"},
};
const size_t EXTENDED_FIELD_COUNT = sizeof(EXTENDED_FIELD_DEFS) / sizeof(EXTENDED_FIELD_DEFS[0]);

/* Selects every field in a master list into a table_filter_t, preserving
 * canonical order. Used both by filter_init_all and internally once
 * requested-name matching has been done. */
static void select_all(table_filter_t *tf, const field_desc_t *defs, size_t count)
{
    for (size_t i = 0; i < count; i++)
        tf->fields[i] = &defs[i];
    tf->count = count;
    tf->enabled = count > 0;
}

/* Computes should_process[c] for every msg_type char in `universe`: true iff
 * the table is enabled and at least one selected field applies to that
 * specific msg_type. */
static void compute_should_process(bool *should_process, const table_filter_t *tf, const char *universe)
{
    if (!tf->enabled)
        return;

    for (const char *mt = universe; *mt != '\0'; mt++)
    {
        for (size_t i = 0; i < tf->count; i++)
        {
            if (strchr(tf->fields[i]->msg_types, *mt) != NULL)
            {
                should_process[(uint8_t)*mt] = true;
                break;
            }
        }
    }
}

static void finalize_filter(field_filter_t *out)
{
    memset(out->should_process, 0, sizeof(out->should_process));
    compute_should_process(out->should_process, &out->admin, ADMIN_MSG_TYPES);
    compute_should_process(out->should_process, &out->market, MARKET_MSG_TYPES);
    compute_should_process(out->should_process, &out->extended, EXTENDED_MSG_TYPES);
}

void filter_init_all(field_filter_t *out)
{
    select_all(&out->admin, ADMIN_FIELD_DEFS, ADMIN_FIELD_COUNT);
    select_all(&out->market, MARKET_FIELD_DEFS, MARKET_FIELD_COUNT);
    select_all(&out->extended, EXTENDED_FIELD_DEFS, EXTENDED_FIELD_COUNT);
    finalize_filter(out);
}

#define MAX_REQUESTED_FIELDS 64

void filter_init_from_csv(const char *csv, field_filter_t *out)
{
    out->admin.count = 0;
    out->market.count = 0;
    out->extended.count = 0;

    char *csv_copy = strdup(csv);
    if (csv_copy == NULL)
    {
        fprintf(stderr, "Error: out of memory parsing --fields\n");
        exit(EXIT_FAILURE);
    }

    char *requested[MAX_REQUESTED_FIELDS];
    bool found[MAX_REQUESTED_FIELDS];
    size_t requested_count = 0;

    char *saveptr = NULL;
    for (char *tok = strtok_r(csv_copy, ",", &saveptr); tok != NULL; tok = strtok_r(NULL, ",", &saveptr))
    {
        if (requested_count >= MAX_REQUESTED_FIELDS)
        {
            fprintf(stderr, "Error: too many fields in --fields (max %d)\n", MAX_REQUESTED_FIELDS);
            free(csv_copy);
            exit(EXIT_FAILURE);
        }
        requested[requested_count] = tok;
        found[requested_count] = false;
        requested_count++;
    }

    struct
    {
        table_filter_t *tf;
        const field_desc_t *defs;
        size_t count;
    } tables[3] = {
        {&out->admin, ADMIN_FIELD_DEFS, ADMIN_FIELD_COUNT},
        {&out->market, MARKET_FIELD_DEFS, MARKET_FIELD_COUNT},
        {&out->extended, EXTENDED_FIELD_DEFS, EXTENDED_FIELD_COUNT},
    };

    for (int t = 0; t < 3; t++)
    {
        for (size_t i = 0; i < tables[t].count; i++)
        {
            const field_desc_t *f = &tables[t].defs[i];
            for (size_t r = 0; r < requested_count; r++)
            {
                if (strcmp(f->name, requested[r]) == 0)
                {
                    if (tables[t].tf->count >= TABLE_FILTER_MAX_FIELDS)
                    {
                        fprintf(stderr, "Error: too many fields selected for one table\n");
                        free(csv_copy);
                        exit(EXIT_FAILURE);
                    }
                    tables[t].tf->fields[tables[t].tf->count++] = f;
                    found[r] = true;
                }
            }
        }
        tables[t].tf->enabled = tables[t].tf->count > 0;
    }

    bool bad = false;
    for (size_t r = 0; r < requested_count; r++)
    {
        if (!found[r])
        {
            fprintf(stderr, "Error: unknown field in --fields: '%s'\n", requested[r]);
            bad = true;
        }
    }

    free(csv_copy);

    if (bad)
        exit(EXIT_FAILURE);

    finalize_filter(out);
}

void filter_build_create_table_sql(const table_filter_t *tf, const char *table_name,
                                   char *buf, size_t bufsize)
{
    int n = snprintf(buf, bufsize, "CREATE TABLE IF NOT EXISTS %s (", table_name);
    for (size_t i = 0; i < tf->count; i++)
    {
        const field_desc_t *f = tf->fields[i];
        n += snprintf(buf + n, (size_t)n < bufsize ? bufsize - (size_t)n : 0,
                      "%s%s %s", (i == 0 ? "" : ", "), f->name, f->sql_type);
    }
    snprintf(buf + n, (size_t)n < bufsize ? bufsize - (size_t)n : 0, ");");
}

void filter_build_select_sql(const table_filter_t *tf, const char *table_name,
                             char *buf, size_t bufsize)
{
    int n = snprintf(buf, bufsize, "SELECT ");
    for (size_t i = 0; i < tf->count; i++)
    {
        const field_desc_t *f = tf->fields[i];
        if (f->ctype == FIELD_U8)
            n += snprintf(buf + n, (size_t)n < bufsize ? bufsize - (size_t)n : 0,
                          "%sCHR(%s) AS %s", (i == 0 ? "" : ", "), f->name, f->name);
        else
            n += snprintf(buf + n, (size_t)n < bufsize ? bufsize - (size_t)n : 0,
                          "%s%s", (i == 0 ? "" : ", "), f->name);
    }
    snprintf(buf + n, (size_t)n < bufsize ? bufsize - (size_t)n : 0, " FROM %s", table_name);
}