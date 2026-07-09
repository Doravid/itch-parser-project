#include "parser.h"
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define ITCH_HEADER_LEN 11
#define MSG_LEN_BYTES 2

/**
 * @brief Memory-maps a file for reading.
 * @param filepath Path to the file.
 * @param out_size Pointer to store the size of the mapped file.
 * @return Pointer to the mapped file buffer.
 */
uint8_t *map_file(const char *filepath, size_t *out_size)
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

/**
 * @brief Unmaps a previously memory-mapped file.
 * @param mapped Pointer to the mapped buffer.
 * @param size Size of the mapped memory region.
 */
void unmap_file(uint8_t *mapped, size_t size)
{
    if (munmap(mapped, size) == -1)
        exit(EXIT_FAILURE);
}

/**
 * @brief Reads a 64-bit integer from big-endian binary data.
 * @param pointer Pointer to the byte array.
 * @return The parsed 64-bit integer.
 */
static inline uint64_t read_int64(const uint8_t *pointer)
{
    return ((uint64_t)pointer[0] << 56) | ((uint64_t)pointer[1] << 48) | ((uint64_t)pointer[2] << 40) | ((uint64_t)pointer[3] << 32) |
           ((uint64_t)pointer[4] << 24) | ((uint64_t)pointer[5] << 16) | ((uint64_t)pointer[6] << 8) | (uint64_t)pointer[7];
}

/**
 * @brief Reads a 32-bit integer from big-endian binary data.
 * @param p Pointer to the byte array.
 * @return The parsed 32-bit integer.
 */
static inline uint32_t read_int32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

/**
 * @brief Reads a 16-bit integer from big-endian binary data.
 * @param p Pointer to the byte array.
 * @return The parsed 16-bit integer.
 */
static inline uint16_t read_int16(const uint8_t *p)
{
    return ((uint16_t)p[0] << 8) | (uint16_t)p[1];
}

/**
 * @brief Reads a 48-bit timestamp from big-endian binary data.
 * @param p Pointer to the byte array.
 * @return The parsed 64-bit integer containing the timestamp.
 */
static inline uint64_t read_timestamp(const uint8_t *p)
{
    return ((uint64_t)p[0] << 40) | ((uint64_t)p[1] << 32) | ((uint64_t)p[2] << 24) | ((uint64_t)p[3] << 16) |
           ((uint64_t)p[4] << 8) | (uint64_t)p[5];
}

/**
 * @brief Parses an ITCH message stream and routes data to the database appenders.
 * @param buffer Pointer to the start of the memory-mapped buffer.
 * @param file_size Total size of the buffer.
 * @param state Pointer to the database state structure.
 * @return The total number of messages processed.
 */
uint64_t parse_stream(const uint8_t *buffer, size_t file_size, db_state_t *state)
{
    const uint8_t *ptr = buffer;
    const uint8_t *end = buffer + file_size;
    uint64_t msg_count = 0;

    struct db_admin_record rec_admin;
    struct db_market_record rec_market;
    struct db_extended_record rec_extended;

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
            memset(&rec_admin, 0, sizeof(rec_admin));
            rec_admin.msg_type = msg_type;
            rec_admin.timestamp = timestamp;

            if (msg_type == 'S')
            {
                rec_admin.event_code = (char)payload[0];
            }
            else if (msg_type == 'R')
            {
                memcpy(rec_admin.stock, payload, 8);
                rec_admin.market_cat = (char)payload[8];
                rec_admin.fin_status = (char)payload[9];
                rec_admin.round_lot_size = read_int32(payload + 10);
                rec_admin.round_lots_only = (char)payload[14];
                rec_admin.issue_class = (char)payload[15];
                memcpy(rec_admin.issue_sub_type, payload + 16, 2);
                rec_admin.authenticity = (char)payload[18];
                rec_admin.short_sale_thresh = (char)payload[19];
                rec_admin.ipo_flag = (char)payload[20];
                rec_admin.luld_tier = (char)payload[21];
                rec_admin.etp_flag = (char)payload[22];
                rec_admin.etp_leverage = read_int32(payload + 23);
                rec_admin.inverse_indicator = (char)payload[27];
            }
            else if (msg_type == 'H')
            {
                memcpy(rec_admin.stock, payload, 8);
                rec_admin.trading_state = (char)payload[8];
                memcpy(rec_admin.reason, payload + 10, 4);
            }
            else if (msg_type == 'Y')
            {
                memcpy(rec_admin.stock, payload, 8);
                rec_admin.reg_sho_action = (char)payload[8];
            }
            else if (msg_type == 'L')
            {
                memcpy(rec_admin.mpid, payload, 4);
                memcpy(rec_admin.stock, payload + 4, 8);
                rec_admin.primary_mm = (char)payload[12];
                rec_admin.mm_mode = (char)payload[13];
                rec_admin.mp_state = (char)payload[14];
            }
            else if (msg_type == 'V')
            {
                rec_admin.level_1 = read_int64(payload);
                rec_admin.level_2 = read_int64(payload + 8);
                rec_admin.level_3 = read_int64(payload + 16);
            }
            else if (msg_type == 'W')
            {
                rec_admin.breached_level = (char)payload[0];
            }
            else if (msg_type == 'K')
            {
                memcpy(rec_admin.stock, payload, 8);
                rec_admin.ipo_release_time = read_int32(payload + 8);
                rec_admin.release_qualifier = (char)payload[12];
                rec_admin.ipo_price = read_int32(payload + 13);
            }
            else if (msg_type == 'J')
            {
                memcpy(rec_admin.stock, payload, 8);
                rec_admin.ref_price = read_int32(payload + 8);
                rec_admin.upper_collar = read_int32(payload + 12);
                rec_admin.lower_collar = read_int32(payload + 16);
                rec_admin.extension = read_int32(payload + 20);
            }
            else if (msg_type == 'h')
            {
                memcpy(rec_admin.stock, payload, 8);
                rec_admin.market_code = (char)payload[8];
                rec_admin.halt_action = (char)payload[9];
            }
            else if (msg_type == 'N')
            {
                memcpy(rec_admin.stock, payload, 8);
                rec_admin.interest_flag = (char)payload[8];
            }
            else if (msg_type == 'O')
            {
                memcpy(rec_admin.stock, payload, 8);
                rec_admin.open_eligibility = (char)payload[8];
                rec_admin.min_allowable_price = read_int32(payload + 9);
                rec_admin.max_allowable_price = read_int32(payload + 13);
                rec_admin.near_exec_price = read_int32(payload + 17);
                rec_admin.near_exec_time = read_int64(payload + 21);
                rec_admin.lower_collar = read_int32(payload + 29);
                rec_admin.upper_collar = read_int32(payload + 33);
            }

            if (db_append_admin(state, &rec_admin) != 0)
                exit(EXIT_FAILURE);
        }
        else if (msg_type == 'A' || msg_type == 'F' || msg_type == 'X' || msg_type == 'D')
        {
            memset(&rec_market, 0, sizeof(rec_market));
            rec_market.msg_type = msg_type;
            rec_market.timestamp = timestamp;

            if (msg_type == 'A')
            {
                rec_market.order_ref = read_int64(payload);
                rec_market.buy_sell = (char)payload[8];
                rec_market.shares = read_int32(payload + 9);
                memcpy(rec_market.stock, payload + 13, 8);
                rec_market.price = read_int32(payload + 21);
            }
            else if (msg_type == 'F')
            {
                rec_market.order_ref = read_int64(payload);
                rec_market.buy_sell = (char)payload[8];
                rec_market.shares = read_int32(payload + 9);
                memcpy(rec_market.stock, payload + 13, 8);
                rec_market.price = read_int32(payload + 21);
                memcpy(rec_market.mpid, payload + 25, 4);
            }
            else if (msg_type == 'X')
            {
                rec_market.order_ref = read_int64(payload);
                rec_market.shares = read_int32(payload + 8);
            }
            else if (msg_type == 'D')
            {
                rec_market.order_ref = read_int64(payload);
            }

            if (db_append_market(state, &rec_market) != 0)
                exit(EXIT_FAILURE);
        }
        else if (msg_type == 'I' || msg_type == 'Q' || msg_type == 'B' ||
                 msg_type == 'E' || msg_type == 'C' || msg_type == 'P' || msg_type == 'U')
        {
            memset(&rec_extended, 0, sizeof(rec_extended));
            rec_extended.msg_type = msg_type;
            rec_extended.timestamp = timestamp;

            if (msg_type == 'Q')
            {
                rec_extended.paired_shares = read_int64(payload);
                memcpy(rec_extended.stock, payload + 8, 8);
                rec_extended.price = read_int32(payload + 16);
                rec_extended.match_number = read_int64(payload + 20);
                rec_extended.cross_type = (char)payload[28];
            }
            else if (msg_type == 'B')
            {
                rec_extended.match_number = read_int64(payload);
            }
            else if (msg_type == 'I')
            {
                rec_extended.paired_shares = read_int64(payload);
                rec_extended.imbalance_shares = read_int64(payload + 8);
                rec_extended.imbalance_dir = (char)payload[16];
                memcpy(rec_extended.stock, payload + 17, 8);
                rec_extended.far_price = read_int32(payload + 25);
                rec_extended.near_price = read_int32(payload + 29);
                rec_extended.current_ref_price = read_int32(payload + 33);
                rec_extended.cross_type = (char)payload[37];
                rec_extended.price_var_ind = (char)payload[38];
            }
            else if (msg_type == 'E')
            {
                rec_extended.order_ref = read_int64(payload);
                rec_extended.shares = read_int32(payload + 8);
                rec_extended.match_number = read_int64(payload + 12);
            }
            else if (msg_type == 'C')
            {
                rec_extended.order_ref = read_int64(payload);
                rec_extended.shares = read_int32(payload + 8);
                rec_extended.match_number = read_int64(payload + 12);
                rec_extended.printable = (char)payload[20];
                rec_extended.price = read_int32(payload + 21);
            }
            else if (msg_type == 'U')
            {
                rec_extended.order_ref = read_int64(payload);
                rec_extended.new_order_ref = read_int64(payload + 8);
                rec_extended.shares = read_int32(payload + 16);
                rec_extended.price = read_int32(payload + 20);
            }
            else if (msg_type == 'P')
            {
                rec_extended.order_ref = read_int64(payload);
                rec_extended.buy_sell = (char)payload[8];
                rec_extended.shares = read_int32(payload + 9);
                memcpy(rec_extended.stock, payload + 13, 8);
                rec_extended.price = read_int32(payload + 21);
                rec_extended.match_number = read_int64(payload + 25);
            }

            if (db_append_extended(state, &rec_extended) != 0)
                exit(EXIT_FAILURE);
        }

        ptr += msg_length;
        msg_count++;
    }

    return msg_count;
}