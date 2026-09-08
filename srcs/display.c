#include"ft_ping.h"

void print_reply(t_ping_config *config, int bytes, double rtt, int ttl) {
    // When the target was given directly as an IP, hostname == resolved_ip:
    // avoid printing the address twice ("from 1.2.3.4 (1.2.3.4)"), like the
    // reference ping does.
    if (strcmp(config->hostname, config->resolved_ip) == 0) {
        printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
               bytes,
               config->resolved_ip,
               config->seq,
               ttl,
               rtt);
    } else {
        printf("%d bytes from %s (%s): icmp_seq=%d ttl=%d time=%.3f ms\n",
               bytes,
               config->hostname,
               config->resolved_ip,
               config->seq,
               ttl,
               rtt);
    }
}

void print_error(t_ping_config *config, const char *from_ip, int type, int code) {
    // "From" must be the host that actually generated the ICMP error
    // (e.g. an intermediate router), not the ping target.
    printf("From %s icmp_seq=%d ",
           from_ip, config->seq);

    switch (type) {
        case ICMP_DEST_UNREACH:
            if (code == ICMP_HOST_UNREACH)
                printf("Destination Host Unreachable");
            else if (code == ICMP_NET_UNREACH)
                printf("Destination Net Unreachable");
            else
                printf("Destination Unreachable");
            break;
        case ICMP_TIME_EXCEEDED:
            printf("Time to live exceeded");
            break;
        default:
            printf("Type=%d Code=%d", type, code);
            break;
    }
    printf("\n");
}
