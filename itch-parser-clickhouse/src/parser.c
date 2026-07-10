#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "parser.h"
#include "db.h"

#define ITCH_HEADER_LEN 11
#define MSG_LEN_BYTES 2

uint8_t *map_file_into_memory(const char *filepath, size_t *out_size)
{
    int fd = open(filepath, O_RDONLY);
    if (fd == -1)
        exit(EXIT_FAILURE);

    struct stat sb;
    if (fstat(fd, &sb) == -1)
    {
        close(fd);
        exit(EXIT_FAILURE);
    }

    *out_size = sb.st_size;
    void *mapped = mmap(NULL, *out_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED)
    {
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
    return (uint8_t *)mapped;
}

void unmap_file(uint8_t *mapped, size_t size)
{
    if (munmap(mapped, size) == -1)
        exit(EXIT_FAILURE);
}

static inline uint64_t read_int64(const uint8_t *pointer)
{
    uint64_t val;
    __builtin_memcpy(&val, pointer, sizeof(val));
    return __builtin_bswap64(val);
}

static inline uint32_t read_int32(const uint8_t *pointer)
{
    uint32_t val;
    __builtin_memcpy(&val, pointer, sizeof(val));
    return __builtin_bswap32(val);
}

static inline uint16_t read_int16(const uint8_t *p)
{
    uint32_t val;
    __builtin_memcpy(&val, p, sizeof(val));
    return __builtin_bswap16(val);
}

static inline uint64_t read_timestamp(const uint8_t *p)
{
    return ((uint64_t)p[0] << 40) | ((uint64_t)p[1] << 32) | ((uint64_t)p[2] << 24) | ((uint64_t)p[3] << 16) |
           ((uint64_t)p[4] << 8) | (uint64_t)p[5];
}

uint64_t parse_itch_stream(const uint8_t *buffer, size_t file_size, db_ctx_t *state)
{
    const uint8_t *ptr = buffer;
    const uint8_t *end = buffer + file_size;
    uint64_t msg_count = 0;

    struct db_record rec;

    while (ptr + MSG_LEN_BYTES <= end)
    {
        uint16_t msg_length = read_int16(ptr);
        ptr += MSG_LEN_BYTES;

        if (ptr + msg_length > end)
            break;

        char msg_type = (char)ptr[0];
        uint64_t timestamp = read_timestamp(ptr + 5);
        const uint8_t *payload = ptr + ITCH_HEADER_LEN;

        if (msg_type == 'S' || msg_type == 'R' || msg_type == 'H' || msg_type == 'Y' ||
            msg_type == 'L' || msg_type == 'V' || msg_type == 'W' || msg_type == 'K' ||
            msg_type == 'J' || msg_type == 'h' || msg_type == 'N' || msg_type == 'O')
        {
            memset(&rec, 0, sizeof(rec));
            rec.msg_type = msg_type;
            rec.timestamp = timestamp;

            if (msg_type == 'S')
            {
                rec.event_code = (char)payload[0];
            }
            else if (msg_type == 'R')
            {
                memcpy(rec.stock, payload, 8);
                rec.market_cat = (char)payload[8];
                rec.fin_status = (char)payload[9];
                rec.round_lot_size = read_int32(payload + 10);
                rec.round_lots_only = (char)payload[14];
                rec.issue_class = (char)payload[15];
                memcpy(rec.issue_sub_type, payload + 16, 2);
                rec.authenticity = (char)payload[18];
                rec.short_sale_thresh = (char)payload[19];
                rec.ipo_flag = (char)payload[20];
                rec.luld_tier = (char)payload[21];
                rec.etp_flag = (char)payload[22];
                rec.etp_leverage = read_int32(payload + 23);
                rec.inverse_indicator = (char)payload[27];
            }
            else if (msg_type == 'H')
            {
                memcpy(rec.stock, payload, 8);
                rec.trading_state = (char)payload[8];
                memcpy(rec.reason, payload + 10, 4);
            }
            else if (msg_type == 'Y')
            {
                memcpy(rec.stock, payload, 8);
                rec.reg_sho_action = (char)payload[8];
            }
            else if (msg_type == 'L')
            {
                memcpy(rec.mpid, payload, 4);
                memcpy(rec.stock, payload + 4, 8);
                rec.primary_mm = (char)payload[12];
                rec.mm_mode = (char)payload[13];
                rec.mp_state = (char)payload[14];
            }
            else if (msg_type == 'V')
            {
                rec.level_1 = read_int64(payload);
                rec.level_2 = read_int64(payload + 8);
                rec.level_3 = read_int64(payload + 16);
            }
            else if (msg_type == 'W')
            {
                rec.breached_level = (char)payload[0];
            }
            else if (msg_type == 'K')
            {
                memcpy(rec.stock, payload, 8);
                rec.ipo_release_time = read_int32(payload + 8);
                rec.release_qualifier = (char)payload[12];
                rec.ipo_price = read_int32(payload + 13);
            }
            else if (msg_type == 'J')
            {
                memcpy(rec.stock, payload, 8);
                rec.ref_price = read_int32(payload + 8);
                rec.upper_collar = read_int32(payload + 12);
                rec.lower_collar = read_int32(payload + 16);
                rec.extension = read_int32(payload + 20);
            }
            else if (msg_type == 'h')
            {
                memcpy(rec.stock, payload, 8);
                rec.market_code = (char)payload[8];
                rec.halt_action = (char)payload[9];
            }
            else if (msg_type == 'N')
            {
                memcpy(rec.stock, payload, 8);
                rec.interest_flag = (char)payload[8];
            }
            else if (msg_type == 'O')
            {
                memcpy(rec.stock, payload, 8);
                rec.open_eligibility = (char)payload[8];
                rec.min_allowable_price = read_int32(payload + 9);
                rec.max_allowable_price = read_int32(payload + 13);
                rec.near_exec_price = read_int32(payload + 17);
                rec.near_exec_time = read_int64(payload + 21);
                rec.lower_collar = read_int32(payload + 29);
                rec.upper_collar = read_int32(payload + 33);
            }

            if (db_append_admin(state, &rec) != 0)
                exit(EXIT_FAILURE);
        }
        else if (msg_type == 'A' || msg_type == 'F' || msg_type == 'X' || msg_type == 'D')
        {
            memset(&rec, 0, sizeof(rec));
            rec.msg_type = msg_type;
            rec.timestamp = timestamp;

            if (msg_type == 'A')
            {
                rec.order_ref = read_int64(payload);
                rec.buy_sell = (char)payload[8];
                rec.shares = read_int32(payload + 9);
                memcpy(rec.stock, payload + 13, 8);
                rec.price = read_int32(payload + 21);
            }
            else if (msg_type == 'F')
            {
                rec.order_ref = read_int64(payload);
                rec.buy_sell = (char)payload[8];
                rec.shares = read_int32(payload + 9);
                memcpy(rec.stock, payload + 13, 8);
                rec.price = read_int32(payload + 21);
                memcpy(rec.mpid, payload + 25, 4);
            }
            else if (msg_type == 'X')
            {
                rec.order_ref = read_int64(payload);
                rec.shares = read_int32(payload + 8);
            }
            else if (msg_type == 'D')
            {
                rec.order_ref = read_int64(payload);
            }

            if (db_append_market(state, &rec) != 0)
                exit(EXIT_FAILURE);
        }
        else if (msg_type == 'I' || msg_type == 'Q' || msg_type == 'B' ||
                 msg_type == 'E' || msg_type == 'C' || msg_type == 'P' || msg_type == 'U')
        {
            memset(&rec, 0, sizeof(rec));
            rec.msg_type = msg_type;
            rec.timestamp = timestamp;

            if (msg_type == 'Q')
            {
                rec.paired_shares = read_int64(payload);
                memcpy(rec.stock, payload + 8, 8);
                rec.price = read_int32(payload + 16);
                rec.match_number = read_int64(payload + 20);
                rec.cross_type = (char)payload[28];
            }
            else if (msg_type == 'B')
            {
                rec.match_number = read_int64(payload);
            }
            else if (msg_type == 'I')
            {
                rec.paired_shares = read_int64(payload);
                rec.imbalance_shares = read_int64(payload + 8);
                rec.imbalance_dir = (char)payload[16];
                memcpy(rec.stock, payload + 17, 8);
                rec.far_price = read_int32(payload + 25);
                rec.near_price = read_int32(payload + 29);
                rec.current_ref_price = read_int32(payload + 33);
                rec.cross_type = (char)payload[37];
                rec.price_var_ind = (char)payload[38];
            }
            else if (msg_type == 'E')
            {
                rec.order_ref = read_int64(payload);
                rec.shares = read_int32(payload + 8);
                rec.match_number = read_int64(payload + 12);
            }
            else if (msg_type == 'C')
            {
                rec.order_ref = read_int64(payload);
                rec.shares = read_int32(payload + 8);
                rec.match_number = read_int64(payload + 12);
                rec.printable = (char)payload[20];
                rec.price = read_int32(payload + 21);
            }
            else if (msg_type == 'U')
            {
                rec.order_ref = read_int64(payload);
                rec.new_order_ref = read_int64(payload + 8);
                rec.shares = read_int32(payload + 16);
                rec.price = read_int32(payload + 20);
            }
            else if (msg_type == 'P')
            {
                rec.order_ref = read_int64(payload);
                rec.buy_sell = (char)payload[8];
                rec.shares = read_int32(payload + 9);
                memcpy(rec.stock, payload + 13, 8);
                rec.price = read_int32(payload + 21);
                rec.match_number = read_int64(payload + 25);
            }

            if (db_append_extended(state, &rec) != 0)
                exit(EXIT_FAILURE);
        }

        ptr += msg_length;
        msg_count++;
    }

    return msg_count;
}