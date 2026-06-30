#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Initializes the database connection and creates required tables.
 * @param db Pointer to the DuckDB database object.
 * @param db_path Path to the database file.
 * @param state Pointer to the state structure holding connections and appenders.
 * @return 0 on success, -1 on failure.
 */
int db_init(duckdb_database *db, const char *db_path, db_state_t *state)
{
    if (duckdb_open(db_path, db) == DuckDBError)
        return -1;
    if (duckdb_connect(*db, &state->conn) == DuckDBError)
        return -1;

    duckdb_query(state->conn,
                 "CREATE TABLE IF NOT EXISTS itch_market (timestamp UBIGINT, msg_type UTINYINT, stock BLOB, mpid BLOB, order_ref UBIGINT, shares UINTEGER, price UINTEGER, buy_sell UTINYINT);",
                 NULL);

    duckdb_query(state->conn,
                 "CREATE TABLE IF NOT EXISTS itch_extended (timestamp UBIGINT, msg_type UTINYINT, stock BLOB, match_number UBIGINT, paired_shares UBIGINT, imbalance_shares UBIGINT, far_price UINTEGER, near_price UINTEGER, current_ref_price UINTEGER, price UINTEGER, cross_type UTINYINT, imbalance_dir UTINYINT, price_var_ind UTINYINT, printable UTINYINT, order_ref UBIGINT, new_order_ref UBIGINT, shares UINTEGER, buy_sell UTINYINT);",
                 NULL);

    duckdb_query(state->conn,
                 "CREATE TABLE IF NOT EXISTS itch_admin (timestamp UBIGINT, msg_type UTINYINT, stock BLOB, reason BLOB, issue_sub_type BLOB, level_1 UBIGINT, level_2 UBIGINT, level_3 UBIGINT, near_exec_time UBIGINT, round_lot_size UINTEGER, etp_leverage UINTEGER, ipo_release_time UINTEGER, ipo_price UINTEGER, ref_price UINTEGER, upper_collar UINTEGER, lower_collar UINTEGER, extension UINTEGER, min_allowable_price UINTEGER, max_allowable_price UINTEGER, near_exec_price UINTEGER, event_code UTINYINT, market_cat UTINYINT, fin_status UTINYINT, round_lots_only UTINYINT, issue_class UTINYINT, authenticity UTINYINT, short_sale_thresh UTINYINT, ipo_flag UTINYINT, luld_tier UTINYINT, etp_flag UTINYINT, inverse_indicator UTINYINT, trading_state UTINYINT, reg_sho_action UTINYINT, primary_mm UTINYINT, mm_mode UTINYINT, mp_state UTINYINT, breached_level UTINYINT, release_qualifier UTINYINT, market_code UTINYINT, halt_action UTINYINT, interest_flag UTINYINT, open_eligibility UTINYINT);",
                 NULL);

    if (duckdb_appender_create(state->conn, NULL, "itch_market", &state->appender_market) == DuckDBError)
        return -1;
    if (duckdb_appender_create(state->conn, NULL, "itch_extended", &state->appender_extended) == DuckDBError)
        return -1;
    if (duckdb_appender_create(state->conn, NULL, "itch_admin", &state->appender_admin) == DuckDBError)
        return -1;

    duckdb_query(state->conn, "PRAGMA preserve_insertion_order=false;", NULL);

    return 0;
}

/**
 * @brief Appends a record to the admin table.
 * @param state Pointer to the database state structure.
 * @param msg Pointer to the parsed admin record.
 * @return 0 on success, -1 on failure.
 */
int db_append_admin(db_state_t *state, const struct db_admin_record *msg)
{
    duckdb_appender app = state->appender_admin;
    duckdb_append_uint64(app, msg->timestamp);
    duckdb_append_uint8(app, (uint8_t)msg->msg_type);
    duckdb_append_blob(app, msg->stock, 8);
    duckdb_append_blob(app, msg->reason, 4);
    duckdb_append_blob(app, msg->issue_sub_type, 2);
    duckdb_append_uint64(app, msg->level_1);
    duckdb_append_uint64(app, msg->level_2);
    duckdb_append_uint64(app, msg->level_3);
    duckdb_append_uint64(app, msg->near_exec_time);
    duckdb_append_uint32(app, msg->round_lot_size);
    duckdb_append_uint32(app, msg->etp_leverage);
    duckdb_append_uint32(app, msg->ipo_release_time);
    duckdb_append_uint32(app, msg->ipo_price);
    duckdb_append_uint32(app, msg->ref_price);
    duckdb_append_uint32(app, msg->upper_collar);
    duckdb_append_uint32(app, msg->lower_collar);
    duckdb_append_uint32(app, msg->extension);
    duckdb_append_uint32(app, msg->min_allowable_price);
    duckdb_append_uint32(app, msg->max_allowable_price);
    duckdb_append_uint32(app, msg->near_exec_price);
    duckdb_append_uint8(app, msg->event_code);
    duckdb_append_uint8(app, msg->market_cat);
    duckdb_append_uint8(app, msg->fin_status);
    duckdb_append_uint8(app, msg->round_lots_only);
    duckdb_append_uint8(app, msg->issue_class);
    duckdb_append_uint8(app, msg->authenticity);
    duckdb_append_uint8(app, msg->short_sale_thresh);
    duckdb_append_uint8(app, msg->ipo_flag);
    duckdb_append_uint8(app, msg->luld_tier);
    duckdb_append_uint8(app, msg->etp_flag);
    duckdb_append_uint8(app, msg->inverse_indicator);
    duckdb_append_uint8(app, msg->trading_state);
    duckdb_append_uint8(app, msg->reg_sho_action);
    duckdb_append_uint8(app, msg->primary_mm);
    duckdb_append_uint8(app, msg->mm_mode);
    duckdb_append_uint8(app, msg->mp_state);
    duckdb_append_uint8(app, msg->breached_level);
    duckdb_append_uint8(app, msg->release_qualifier);
    duckdb_append_uint8(app, msg->market_code);
    duckdb_append_uint8(app, msg->halt_action);
    duckdb_append_uint8(app, msg->interest_flag);
    duckdb_append_uint8(app, msg->open_eligibility);
    return duckdb_appender_end_row(app) == DuckDBError ? -1 : 0;
}

/**
 * @brief Appends a record to the market table.
 * @param state Pointer to the database state structure.
 * @param msg Pointer to the parsed market record.
 * @return 0 on success, -1 on failure.
 */
int db_append_market(db_state_t *state, const struct db_market_record *msg)
{
    duckdb_appender app = state->appender_market;
    duckdb_append_uint64(app, msg->timestamp);
    duckdb_append_uint8(app, (uint8_t)msg->msg_type);
    duckdb_append_blob(app, msg->stock, 8);
    duckdb_append_blob(app, msg->mpid, 4);
    duckdb_append_uint64(app, msg->order_ref);
    duckdb_append_uint32(app, msg->shares);
    duckdb_append_uint32(app, msg->price);
    duckdb_append_uint8(app, msg->buy_sell);
    return duckdb_appender_end_row(app) == DuckDBError ? -1 : 0;
}

/**
 * @brief Appends a record to the extended table.
 * @param state Pointer to the database state structure.
 * @param msg Pointer to the parsed extended record.
 * @return 0 on success, -1 on failure.
 */
int db_append_extended(db_state_t *state, const struct db_extended_record *msg)
{
    duckdb_appender app = state->appender_extended;
    duckdb_append_uint64(app, msg->timestamp);
    duckdb_append_uint8(app, (uint8_t)msg->msg_type);
    duckdb_append_blob(app, msg->stock, 8);
    duckdb_append_uint64(app, msg->match_number);
    duckdb_append_uint64(app, msg->paired_shares);
    duckdb_append_uint64(app, msg->imbalance_shares);
    duckdb_append_uint32(app, msg->far_price);
    duckdb_append_uint32(app, msg->near_price);
    duckdb_append_uint32(app, msg->current_ref_price);
    duckdb_append_uint32(app, msg->price);
    duckdb_append_uint8(app, msg->cross_type);
    duckdb_append_uint8(app, msg->imbalance_dir);
    duckdb_append_uint8(app, msg->price_var_ind);
    duckdb_append_uint8(app, msg->printable);
    duckdb_append_uint64(app, msg->order_ref);
    duckdb_append_uint64(app, msg->new_order_ref);
    duckdb_append_uint32(app, msg->shares);
    duckdb_append_uint8(app, msg->buy_sell);
    return duckdb_appender_end_row(app) == DuckDBError ? -1 : 0;
}

/**
 * @brief Destroys appenders, creates view, and closes the database.
 * @param db The DuckDB database object.
 * @param state Pointer to the database state structure.
 */
void db_close(duckdb_database db, db_state_t *state)
{
    duckdb_appender_destroy(&state->appender_market);
    duckdb_appender_destroy(&state->appender_extended);
    duckdb_appender_destroy(&state->appender_admin);
    duckdb_disconnect(&state->conn);

    duckdb_connection conn;
    if (duckdb_connect(db, &conn) == DuckDBError)
        return;

    duckdb_query(conn,
                 "CREATE VIEW itch_unified AS "
                 "SELECT timestamp, CHR(msg_type) AS msg_type, stock, mpid, order_ref, NULL AS new_order_ref, NULL AS match_number, shares, NULL AS paired_shares, NULL AS imbalance_shares, price, NULL AS far_price, NULL AS near_price, NULL AS current_ref_price, CHR(buy_sell) AS buy_sell, NULL AS printable, NULL AS cross_type, NULL AS imbalance_dir, NULL AS price_var_ind, NULL AS reason, NULL AS issue_sub_type, NULL AS level_1, NULL AS level_2, NULL AS level_3, NULL AS near_exec_time, NULL AS round_lot_size, NULL AS etp_leverage, NULL AS ipo_release_time, NULL AS ipo_price, NULL AS ref_price, NULL AS upper_collar, NULL AS lower_collar, NULL AS extension, NULL AS min_allowable_price, NULL AS max_allowable_price, NULL AS near_exec_price, NULL AS event_code, NULL AS market_cat, NULL AS fin_status, NULL AS round_lots_only, NULL AS issue_class, NULL AS authenticity, NULL AS short_sale_thresh, NULL AS ipo_flag, NULL AS luld_tier, NULL AS etp_flag, NULL AS inverse_indicator, NULL AS trading_state, NULL AS reg_sho_action, NULL AS primary_mm, NULL AS mm_mode, NULL AS mp_state, NULL AS breached_level, NULL AS release_qualifier, NULL AS market_code, NULL AS halt_action, NULL AS interest_flag, NULL AS open_eligibility FROM itch_market "
                 "UNION ALL "
                 "SELECT timestamp, CHR(msg_type) AS msg_type, stock, NULL AS mpid, order_ref, new_order_ref, match_number, shares, paired_shares, imbalance_shares, price, far_price, near_price, current_ref_price, CHR(buy_sell) AS buy_sell, CHR(printable) AS printable, CHR(cross_type) AS cross_type, CHR(imbalance_dir) AS imbalance_dir, CHR(price_var_ind) AS price_var_ind, NULL AS reason, NULL AS issue_sub_type, NULL AS level_1, NULL AS level_2, NULL AS level_3, NULL AS near_exec_time, NULL AS round_lot_size, NULL AS etp_leverage, NULL AS ipo_release_time, NULL AS ipo_price, NULL AS ref_price, NULL AS upper_collar, NULL AS lower_collar, NULL AS extension, NULL AS min_allowable_price, NULL AS max_allowable_price, NULL AS near_exec_price, NULL AS event_code, NULL AS market_cat, NULL AS fin_status, NULL AS round_lots_only, NULL AS issue_class, NULL AS authenticity, NULL AS short_sale_thresh, NULL AS ipo_flag, NULL AS luld_tier, NULL AS etp_flag, NULL AS inverse_indicator, NULL AS trading_state, NULL AS reg_sho_action, NULL AS primary_mm, NULL AS mm_mode, NULL AS mp_state, NULL AS breached_level, NULL AS release_qualifier, NULL AS market_code, NULL AS halt_action, NULL AS interest_flag, NULL AS open_eligibility FROM itch_extended "
                 "UNION ALL "
                 "SELECT timestamp, CHR(msg_type) AS msg_type, stock, mpid, NULL AS order_ref, NULL AS new_order_ref, NULL AS match_number, NULL AS shares, NULL AS paired_shares, NULL AS imbalance_shares, NULL AS price, NULL AS far_price, NULL AS near_price, NULL AS current_ref_price, NULL AS buy_sell, NULL AS printable, NULL AS cross_type, NULL AS imbalance_dir, NULL AS price_var_ind, reason, issue_sub_type, level_1, level_2, level_3, near_exec_time, round_lot_size, etp_leverage, ipo_release_time, ipo_price, ref_price, upper_collar, lower_collar, extension, min_allowable_price, max_allowable_price, near_exec_price, CHR(event_code) AS event_code, CHR(market_cat) AS market_cat, CHR(fin_status) AS fin_status, CHR(round_lots_only) AS round_lots_only, CHR(issue_class) AS issue_class, CHR(authenticity) AS authenticity, CHR(short_sale_thresh) AS short_sale_thresh, CHR(ipo_flag) AS ipo_flag, CHR(luld_tier) AS luld_tier, CHR(etp_flag) AS etp_flag, CHR(inverse_indicator) AS inverse_indicator, CHR(trading_state) AS trading_state, CHR(reg_sho_action) AS reg_sho_action, CHR(primary_mm) AS primary_mm, CHR(mm_mode) AS mm_mode, CHR(mp_state) AS mp_state, CHR(breached_level) AS breached_level, CHR(release_qualifier) AS release_qualifier, CHR(market_code) AS market_code, CHR(halt_action) AS halt_action, CHR(interest_flag) AS interest_flag, CHR(open_eligibility) AS open_eligibility FROM itch_admin;",
                 NULL);

    duckdb_disconnect(&conn);
    duckdb_close(&db);
}