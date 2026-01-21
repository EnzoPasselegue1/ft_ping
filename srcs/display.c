#include"ft_ping.h"

void print_reply(t_ping_config *config, int bytes, double rtt, int ttl) {
    printf("%d bytes from %s (%s): icmp_seq=%d ttl=%d time=%.3f ms\n",
           bytes,
           config->hostname,
           config->resolved_ip,
           config->seq,
           ttl,
           rtt);
}

void print_error(t_ping_config *config, int type, int code) {
    printf("From %s icmp_seq=%d ",
           config->resolved_ip, config->seq);

    switch (type) {
        case ICMP_DEST_UNREACH:
            printf("Destination Unreachable");
            if (code == ICMP_HOST_UNREACH)
                printf(" (Host Unreachable)");
            else if (code == ICMP_NET_UNREACH)
                printf(" (Network Unreachable)");
            break;
        case ICMP_TIME_EXCEEDED:
            printf("Time to Live exceeded");
            break;
        default:
            printf("Type=%d Code=%d", type, code);
            break;
    }
    printf("\n");
}
