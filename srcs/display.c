#include"ft_ping.h"

void print_reply(t_ping_config *config, int bytes, double rtt, int ttl) {
    printf("%d bytes from%s (%s): icmp_seq=%d ttl=%d time=%.3f ms\n",
           bytes,
           config->hostname,
           config->resolved_ip,
           config->seq,
           ttl,
           rtt);
}

void print_error(t_ping_config *config, int type, int code) {
    printf("From%s icmp_seq=%d ",
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

void update_stats(t_ping_stats *stats, double rtt) {
    if (rtt < stats->min_rtt)
        stats->min_rtt = rtt;
    if (rtt > stats->max_rtt)
        stats->max_rtt = rtt;

    stats->sum_rtt += rtt;
    stats->sum_rtt_sq += rtt * rtt;
}

double calculate_rtt(struct timeval *start, struct timeval *end) {
    double start_ms = start->tv_sec * 1000.0 + start->tv_usec / 1000.0;
    double end_ms = end->tv_sec * 1000.0 + end->tv_usec / 1000.0;
    return end_ms - start_ms;
}