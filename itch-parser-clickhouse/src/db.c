#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_CAPACITY (4 * 1024 * 1024)

static void execute_ddl(const char *query)
{
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8123/");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, query);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
}

int db_global_init(void **db, const char *db_path)
{
    curl_global_init(CURL_GLOBAL_ALL);

    execute_ddl("DROP VIEW IF EXISTS itch_unified;");
    execute_ddl("DROP TABLE IF EXISTS itch_market;");
    execute_ddl("DROP TABLE IF EXISTS itch_extended;");
    execute_ddl("DROP TABLE IF EXISTS itch_admin;");

    execute_ddl("CREATE TABLE itch_market (timestamp UInt64, msg_type UInt8, stock FixedString(8), mpid FixedString(4), order_ref UInt64, shares UInt32, price UInt32, buy_sell UInt8) ENGINE = MergeTree ORDER BY timestamp;");
    execute_ddl("CREATE TABLE itch_extended (timestamp UInt64, msg_type UInt8, stock FixedString(8), match_number UInt64, paired_shares UInt64, imbalance_shares UInt64, far_price UInt32, near_price UInt32, current_ref_price UInt32, price UInt32, cross_type UInt8, imbalance_dir UInt8, price_var_ind UInt8, printable UInt8, order_ref UInt64, new_order_ref UInt64, shares UInt32, buy_sell UInt8) ENGINE = MergeTree ORDER BY timestamp;");
    execute_ddl("CREATE TABLE itch_admin (timestamp UInt64, msg_type UInt8, stock FixedString(8), reason FixedString(4), issue_sub_type FixedString(2), level_1 UInt64, level_2 UInt64, level_3 UInt64, near_exec_time UInt64, round_lot_size UInt32, etp_leverage UInt32, ipo_release_time UInt32, ipo_price UInt32, ref_price UInt32, upper_collar UInt32, lower_collar UInt32, extension UInt32, min_allowable_price UInt32, max_allowable_price UInt32, near_exec_price UInt32, event_code UInt8, market_cat UInt8, fin_status UInt8, round_lots_only UInt8, issue_class UInt8, authenticity UInt8, short_sale_thresh UInt8, ipo_flag UInt8, luld_tier UInt8, etp_flag UInt8, inverse_indicator UInt8, trading_state UInt8, reg_sho_action UInt8, primary_mm UInt8, mm_mode UInt8, mp_state UInt8, breached_level UInt8, release_qualifier UInt8, market_code UInt8, halt_action UInt8, interest_flag UInt8, open_eligibility UInt8) ENGINE = MergeTree ORDER BY timestamp;");

    return 0;
}

static CURL *init_curl_worker(const char *url)
{
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    return curl;
}

int db_worker_init(db_ctx_t *ctx, void *db)
{
    ctx->curl_market = init_curl_worker("http://localhost:8123/?query=INSERT%20INTO%20itch_market%20FORMAT%20RowBinary&async_insert=1&wait_for_async_insert=0");
    ctx->curl_extended = init_curl_worker("http://localhost:8123/?query=INSERT%20INTO%20itch_extended%20FORMAT%20RowBinary&async_insert=1&wait_for_async_insert=0");
    ctx->curl_admin = init_curl_worker("http://localhost:8123/?query=INSERT%20INTO%20itch_admin%20FORMAT%20RowBinary&async_insert=1&wait_for_async_insert=0");

    ctx->market_buf = malloc(BUF_CAPACITY);
    ctx->extended_buf = malloc(BUF_CAPACITY);
    ctx->admin_buf = malloc(BUF_CAPACITY);

    ctx->market_ptr = ctx->market_buf;
    ctx->extended_ptr = ctx->extended_buf;
    ctx->admin_ptr = ctx->admin_buf;

    return 0;
}

static inline void w8(char **p, uint8_t v)
{
    **p = v;
    *p += 1;
}
static inline void w32(char **p, uint32_t v)
{
    memcpy(*p, &v, 4);
    *p += 4;
}
static inline void w64(char **p, uint64_t v)
{
    memcpy(*p, &v, 8);
    *p += 8;
}
static inline void wbytes(char **p, const void *data, size_t len)
{
    memcpy(*p, data, len);
    *p += len;
}

static inline void check_flush(CURL *curl, char **buf_start, char **buf_ptr)
{
    size_t len = *buf_ptr - *buf_start;
    if (len >= BUF_CAPACITY - 256)
    {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, *buf_start);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, len);
        curl_easy_perform(curl);
        *buf_ptr = *buf_start;
    }
}

int db_append_market(db_ctx_t *state, const struct db_record *msg)
{
    w64(&state->market_ptr, msg->timestamp);
    w8(&state->market_ptr, msg->msg_type);
    wbytes(&state->market_ptr, msg->stock, 8);
    wbytes(&state->market_ptr, msg->mpid, 4);
    w64(&state->market_ptr, msg->order_ref);
    w32(&state->market_ptr, msg->shares);
    w32(&state->market_ptr, msg->price);
    w8(&state->market_ptr, msg->buy_sell);

    check_flush(state->curl_market, &state->market_buf, &state->market_ptr);
    return 0;
}

int db_append_extended(db_ctx_t *state, const struct db_record *msg)
{
    w64(&state->extended_ptr, msg->timestamp);
    w8(&state->extended_ptr, msg->msg_type);
    wbytes(&state->extended_ptr, msg->stock, 8);
    w64(&state->extended_ptr, msg->match_number);
    w64(&state->extended_ptr, msg->paired_shares);
    w64(&state->extended_ptr, msg->imbalance_shares);
    w32(&state->extended_ptr, msg->far_price);
    w32(&state->extended_ptr, msg->near_price);
    w32(&state->extended_ptr, msg->current_ref_price);
    w32(&state->extended_ptr, msg->price);
    w8(&state->extended_ptr, msg->cross_type);
    w8(&state->extended_ptr, msg->imbalance_dir);
    w8(&state->extended_ptr, msg->price_var_ind);
    w8(&state->extended_ptr, msg->printable);
    w64(&state->extended_ptr, msg->order_ref);
    w64(&state->extended_ptr, msg->new_order_ref);
    w32(&state->extended_ptr, msg->shares);
    w8(&state->extended_ptr, msg->buy_sell);

    check_flush(state->curl_extended, &state->extended_buf, &state->extended_ptr);
    return 0;
}

int db_append_admin(db_ctx_t *state, const struct db_record *msg)
{
    w64(&state->admin_ptr, msg->timestamp);
    w8(&state->admin_ptr, msg->msg_type);
    wbytes(&state->admin_ptr, msg->stock, 8);
    wbytes(&state->admin_ptr, msg->reason, 4);
    wbytes(&state->admin_ptr, msg->issue_sub_type, 2);
    w64(&state->admin_ptr, msg->level_1);
    w64(&state->admin_ptr, msg->level_2);
    w64(&state->admin_ptr, msg->level_3);
    w64(&state->admin_ptr, msg->near_exec_time);
    w32(&state->admin_ptr, msg->round_lot_size);
    w32(&state->admin_ptr, msg->etp_leverage);
    w32(&state->admin_ptr, msg->ipo_release_time);
    w32(&state->admin_ptr, msg->ipo_price);
    w32(&state->admin_ptr, msg->ref_price);
    w32(&state->admin_ptr, msg->upper_collar);
    w32(&state->admin_ptr, msg->lower_collar);
    w32(&state->admin_ptr, msg->extension);
    w32(&state->admin_ptr, msg->min_allowable_price);
    w32(&state->admin_ptr, msg->max_allowable_price);
    w32(&state->admin_ptr, msg->near_exec_price);
    w8(&state->admin_ptr, msg->event_code);
    w8(&state->admin_ptr, msg->market_cat);
    w8(&state->admin_ptr, msg->fin_status);
    w8(&state->admin_ptr, msg->round_lots_only);
    w8(&state->admin_ptr, msg->issue_class);
    w8(&state->admin_ptr, msg->authenticity);
    w8(&state->admin_ptr, msg->short_sale_thresh);
    w8(&state->admin_ptr, msg->ipo_flag);
    w8(&state->admin_ptr, msg->luld_tier);
    w8(&state->admin_ptr, msg->etp_flag);
    w8(&state->admin_ptr, msg->inverse_indicator);
    w8(&state->admin_ptr, msg->trading_state);
    w8(&state->admin_ptr, msg->reg_sho_action);
    w8(&state->admin_ptr, msg->primary_mm);
    w8(&state->admin_ptr, msg->mm_mode);
    w8(&state->admin_ptr, msg->mp_state);
    w8(&state->admin_ptr, msg->breached_level);
    w8(&state->admin_ptr, msg->release_qualifier);
    w8(&state->admin_ptr, msg->market_code);
    w8(&state->admin_ptr, msg->halt_action);
    w8(&state->admin_ptr, msg->interest_flag);
    w8(&state->admin_ptr, msg->open_eligibility);

    check_flush(state->curl_admin, &state->admin_buf, &state->admin_ptr);
    return 0;
}

int db_append_order(db_ctx_t *ctx, const struct db_record *msg)
{
    if (msg->msg_type == 'A' || msg->msg_type == 'F' || msg->msg_type == 'X' || msg->msg_type == 'D')
        return db_append_market(ctx, msg);
    if (msg->msg_type == 'S' || msg->msg_type == 'R' || msg->msg_type == 'H' || msg->msg_type == 'Y' || msg->msg_type == 'L' || msg->msg_type == 'V' || msg->msg_type == 'W' || msg->msg_type == 'K' || msg->msg_type == 'J' || msg->msg_type == 'h' || msg->msg_type == 'N' || msg->msg_type == 'O')
        return db_append_admin(ctx, msg);
    return db_append_extended(ctx, msg);
}

static void force_flush(CURL *curl, char *buf_start, char *buf_ptr)
{
    size_t len = buf_ptr - buf_start;
    if (len > 0)
    {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buf_start);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, len);
        curl_easy_perform(curl);
    }
}

void db_worker_flush(db_ctx_t *ctx)
{
    force_flush(ctx->curl_market, ctx->market_buf, ctx->market_ptr);
    force_flush(ctx->curl_extended, ctx->extended_buf, ctx->extended_ptr);
    force_flush(ctx->curl_admin, ctx->admin_buf, ctx->admin_ptr);
}

void db_worker_close(db_ctx_t *ctx)
{
    db_worker_flush(ctx);
    curl_easy_cleanup(ctx->curl_market);
    curl_easy_cleanup(ctx->curl_extended);
    curl_easy_cleanup(ctx->curl_admin);
    free(ctx->market_buf);
    free(ctx->extended_buf);
    free(ctx->admin_buf);
}

void db_global_finalize(void *db)
{
    execute_ddl("CREATE VIEW itch_unified AS SELECT timestamp, char(msg_type) AS msg_type, stock, mpid, order_ref, NULL AS new_order_ref, NULL AS match_number, shares, NULL AS paired_shares, NULL AS imbalance_shares, price, NULL AS far_price, NULL AS near_price, NULL AS current_ref_price, char(buy_sell) AS buy_sell, NULL AS printable, NULL AS cross_type, NULL AS imbalance_dir, NULL AS price_var_ind, NULL AS reason, NULL AS issue_sub_type, NULL AS level_1, NULL AS level_2, NULL AS level_3, NULL AS near_exec_time, NULL AS round_lot_size, NULL AS etp_leverage, NULL AS ipo_release_time, NULL AS ipo_price, NULL AS ref_price, NULL AS upper_collar, NULL AS lower_collar, NULL AS extension, NULL AS min_allowable_price, NULL AS max_allowable_price, NULL AS near_exec_price, NULL AS event_code, NULL AS market_cat, NULL AS fin_status, NULL AS round_lots_only, NULL AS issue_class, NULL AS authenticity, NULL AS short_sale_thresh, NULL AS ipo_flag, NULL AS luld_tier, NULL AS etp_flag, NULL AS inverse_indicator, NULL AS trading_state, NULL AS reg_sho_action, NULL AS primary_mm, NULL AS mm_mode, NULL AS mp_state, NULL AS breached_level, NULL AS release_qualifier, NULL AS market_code, NULL AS halt_action, NULL AS interest_flag, NULL AS open_eligibility FROM itch_market UNION ALL SELECT timestamp, char(msg_type), stock, NULL, NULL, new_order_ref, match_number, shares, paired_shares, imbalance_shares, price, far_price, near_price, current_ref_price, char(buy_sell), char(printable), char(cross_type), char(imbalance_dir), char(price_var_ind), NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL FROM itch_extended UNION ALL SELECT timestamp, char(msg_type), stock, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, reason, issue_sub_type, level_1, level_2, level_3, near_exec_time, round_lot_size, etp_leverage, ipo_release_time, ipo_price, ref_price, upper_collar, lower_collar, extension, min_allowable_price, max_allowable_price, near_exec_price, char(event_code), char(market_cat), char(fin_status), char(round_lots_only), char(issue_class), char(authenticity), char(short_sale_thresh), char(ipo_flag), char(luld_tier), char(etp_flag), char(inverse_indicator), char(trading_state), char(reg_sho_action), char(primary_mm), char(mm_mode), char(mp_state), char(breached_level), char(release_qualifier), char(market_code), char(halt_action), char(interest_flag), char(open_eligibility) FROM itch_admin;");
    curl_global_cleanup();
}