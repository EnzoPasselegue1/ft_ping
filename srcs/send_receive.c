#include "ft_ping.h"

int     send_ping(t_ping_config *config, t_ping_stats *stats)
{
    t_icmp_packet packet;
    ssize_t bytes_sent;

    build_icmp_packet(&packet, config);

    bytes_sent = sendto(config->sockfd, &packet, sizeof(packet), 0,
                        (struct sockaddr *)&config->dest_addr,
                        sizeof(config->dest_addr));
    if (bytes_sent < 0 || bytes_sent != sizeof(packet)) {
        if (config->verbose)
            perror("ft_ping: sendto");
        return -1;
    }

    stats->transmitted++;
    return 0;
}

int     receive_ping(t_ping_config *config, t_ping_stats *stats)
{
    char buffer[1024];
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    ssize_t bytes_received;

    bytes_received = recvfrom(config->sockfd, buffer, sizeof(buffer), 0,
                              (struct sockaddr *)&addr, &addr_len);
    if (bytes_received < 0) {
        if (config->verbose && (errno != EAGAIN && errno != EWOULDBLOCK))
            perror("ft_ping: recvfrom");
        return -1;
    }

    struct iphdr *ip_header = (struct iphdr *)buffer;
    t_icmp_packet *icmp_packet = (t_icmp_packet *)(buffer + (ip_header->ihl * 4));

    if (icmp_packet->header.type == ICMP_ECHOREPLY &&
        ntohs(icmp_packet->header.un.echo.id) == config->pid) {
        struct timeval *send_time = (struct timeval *)icmp_packet->data;
        struct timeval recv_time, rtt_time;
        gettimeofday(&recv_time, NULL);

        timersub(&recv_time, send_time, &rtt_time);
        double rtt = rtt_time.tv_sec * 1000.0 + rtt_time.tv_usec / 1000.0;

        stats->received++;
        stats->sum_rtt += rtt;
        stats->sum_rtt_sq += rtt * rtt;
        if (stats->min_rtt < 0 || rtt < stats->min_rtt)
            stats->min_rtt = rtt;
        if (rtt > stats->max_rtt)
            stats->max_rtt = rtt;

        if (config->verbose) {
            char addr_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr.sin_addr, addr_str, sizeof(addr_str));
            printf("%ld bytes from %s: icmp_seq=%d ttl=%d time=%.2f ms\n",
                   bytes_received - (ip_header->ihl * 4),
                   addr_str,
                   ntohs(icmp_packet->header.un.echo.sequence),
                   ip_header->ttl,
                   rtt);
        }
        return 0;
    }

    return -1;
}
