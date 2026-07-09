#include "../src/parser.h"
#include <assert.h>
#include <string.h>

int main(void)
{
    uint8_t mock_packet[36] = {0};
    mock_packet[0] = 'A';
    mock_packet[3] = 0x01;
    memcpy(&mock_packet[24], "AAPL    ", 8);

    struct itch_order_add msg = {0};
    parse_itch_packet(mock_packet, &msg);

    assert(msg.type == 'A');
    assert(msg.tracking_number == 0x0100);
    assert(memcmp(msg.stock, "AAPL    ", 8) == 0);

    return 0;
}