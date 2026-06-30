#pragma once
#include <duckdb.h>
#include <stdint.h>

struct db_admin_record
{
    uint64_t timestamp;
    char msg_type;
    char stock[8];
    char mpid[4];
    char reason[4];
    char issue_sub_type[2];
    uint64_t level_1;
    uint64_t level_2;
    uint64_t level_3;
    uint64_t near_exec_time;
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
    char interest_flag;
    char open_eligibility;
};

struct db_market_record
{
    uint64_t timestamp;
    char msg_type;
    char stock[8];
    char mpid[4];
    uint64_t order_ref;
    uint32_t shares;
    uint32_t price;
    char buy_sell;
};

struct db_extended_record
{
    uint64_t timestamp;
    char msg_type;
    char stock[8];
    uint64_t match_number;
    uint64_t paired_shares;
    uint64_t imbalance_shares;
    uint32_t far_price;
    uint32_t near_price;
    uint32_t current_ref_price;
    uint32_t price;
    char cross_type;
    char imbalance_dir;
    char price_var_ind;
    char printable;
    uint64_t order_ref;
    uint64_t new_order_ref;
    uint32_t shares;
    char buy_sell;
};

typedef struct
{
    duckdb_connection conn;
    duckdb_appender appender_admin;
    duckdb_appender appender_market;
    duckdb_appender appender_extended;
} db_state_t;

int db_init(duckdb_database *db, const char *db_path, db_state_t *state);
int db_append_admin(db_state_t *state, const struct db_admin_record *msg);
int db_append_market(db_state_t *state, const struct db_market_record *msg);
int db_append_extended(db_state_t *state, const struct db_extended_record *msg);
void db_close(duckdb_database db, db_state_t *state);
