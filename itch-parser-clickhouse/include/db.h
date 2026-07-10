#pragma once
#include <stdint.h>
#include <stddef.h>
#include <curl/curl.h>

typedef struct
{
    CURL *curl_market;
    CURL *curl_extended;
    CURL *curl_admin;

    char *market_buf;
    char *extended_buf;
    char *admin_buf;

    char *market_ptr;
    char *extended_ptr;
    char *admin_ptr;
} db_ctx_t;

struct db_record
{
    uint64_t timestamp;
    uint64_t order_ref;
    uint64_t new_order_ref;
    uint64_t match_number;
    uint64_t shares;
    uint64_t paired_shares;
    uint64_t imbalance_shares;
    uint64_t level_1;
    uint64_t level_2;
    uint64_t level_3;
    uint64_t near_exec_time;
    uint32_t price;
    uint32_t far_price;
    uint32_t near_price;
    uint32_t current_ref_price;
    uint32_t round_lot_size;
    uint32_t etp_leverage;
    uint32_t ipo_release_time;
    uint32_t ipo_price;
    uint32_t ref_price;
    uint32_t upper_collar;
    uint32_t lower_collar;
    uint32_t extension;
    uint32_t min_allowable_price;
    uint32_t max_allowable_price;
    uint32_t near_exec_price;
    uint8_t stock[8];
    uint8_t mpid[4];
    uint8_t reason[4];
    uint8_t issue_sub_type[2];
    char msg_type;
    char event_code;
    char market_cat;
    char fin_status;
    char round_lots_only;
    char issue_class;
    char authenticity;
    char short_sale_thresh;
    char ipo_flag;
    char luld_tier;
    char etp_flag;
    char inverse_indicator;
    char trading_state;
    char reg_sho_action;
    char primary_mm;
    char mm_mode;
    char mp_state;
    char breached_level;
    char release_qualifier;
    char market_code;
    char halt_action;
    char buy_sell;
    char printable;
    char cross_type;
    char imbalance_dir;
    char price_var_ind;
    char interest_flag;
    char open_eligibility;
};

int db_global_init(void **db, const char *db_path);
int db_worker_init(db_ctx_t *ctx, void *db);
int db_append_order(db_ctx_t *ctx, const struct db_record *msg);
void db_worker_flush(db_ctx_t *ctx);
void db_worker_close(db_ctx_t *ctx);
void db_global_finalize(void *db);

int db_append_extended(db_ctx_t *state, const struct db_record *msg);
int db_append_market(db_ctx_t *state, const struct db_record *msg);
int db_append_admin(db_ctx_t *state, const struct db_record *msg);