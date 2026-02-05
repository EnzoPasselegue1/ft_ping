#include"ft_ping.h"

int send_ping(t_ping_config *config, t_ping_stats *stats) {
    t_icmp_packet packet;
    int sent;

    build_icmp_packet(&packet, config);

    sent = sendto(config->sockfd, &packet, sizeof(packet), 0,
                  (struct sockaddr *)&config->dest_addr,
                  sizeof(config->dest_addr));

    if (sent < 0) {
        perror("ft_ping: sendto");
        return -1;
    }

    stats->transmitted++;
    return 0;
}

int receive_ping(t_ping_config *config, t_ping_stats *stats,
                 struct timeval *start) {
    char buffer[1024];
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    struct timeval end;
    int received;
    int max_attempts = 10;
    int attempts = 0;

    while (attempts++ < max_attempts) {
        fromlen = sizeof(from);

        received = recvfrom(config->sockfd, buffer, sizeof(buffer), 0,
                            (struct sockaddr *)&from, &fromlen);

        if (received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return -1;
            }
            if (config->verbose)
                perror("ft_ping: recvfrom");
            return -1;
        }

        gettimeofday(&end, NULL);

        struct iphdr *ip_hdr = (struct iphdr *)buffer;
        int ip_hdr_len = ip_hdr->ihl * 4;
        struct icmphdr *icmp_hdr = (struct icmphdr *)(buffer + ip_hdr_len);

        // Ignorer les echo requests
        if (icmp_hdr->type == ICMP_ECHO) {
            continue;
        }

        // Ignorer les paquets qui ne sont pas pour nous
        if (icmp_hdr->type != ICMP_ECHOREPLY ||
            ntohs(icmp_hdr->un.echo.id) != config->pid ||
            ntohs(icmp_hdr->un.echo.sequence) != config->seq) {
            continue;
        }

        double rtt = calculate_rtt(start, &end);
        update_stats(stats, rtt);
        print_reply(config, received - ip_hdr_len, rtt, ip_hdr->ttl);

        stats->received++;
        return 0;
    }

    return -1;
}
