#include "ft_ping.h"

uint16_t calculate_checksum(void *data, int len) {
    uint16_t *buf = (uint16_t *)data;
    uint32_t sum = 0;

    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }

    if (len == 1)
        sum += *(uint8_t *)buf;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    return (uint16_t)(~sum);
}

void    build_icmp_packet(t_icmp_packet *packet, t_ping_config *config)
{
    memset(packet, 0, sizeof(t_icmp_packet));

    packet->header.type = ICMP_ECHO;
    packet->header.code = 0;
    packet->header.un.echo.id = htons(config->pid);
    packet->header.un.echo.sequence = htons(config->seq);

    struct timeval *tv = (struct timeval *)packet->data;
    gettimeofday(tv, NULL);
    memcpy(packet->data, tv, sizeof(*tv));

    for (int i = sizeof(*tv); i < DATA_SIZE; i++)
        packet->data[i] = i;

    packet->header.checksum = 0;
    packet->header.checksum = calculate_checksum(packet, sizeof(t_icmp_packet));
}
