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
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                // Timeout, or interrupted by a signal (e.g. CTRL+C): not an
                // error worth reporting, just stop waiting for this probe.
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

        // Erreurs ICMP (ex: routeur qui répond "Destination Unreachable" ou
        // "Time Exceeded" au lieu d'un echo reply). Le paquet original que
        // nous avons envoyé est ré-embarqué juste après le header ICMP
        // d'erreur (IP header + 8 premiers octets, dont l'id ICMP) : on s'en
        // sert pour vérifier que l'erreur nous concerne bien avant de
        // l'afficher.
        if (icmp_hdr->type == ICMP_DEST_UNREACH ||
            icmp_hdr->type == ICMP_TIME_EXCEEDED) {
            int inner_off = ip_hdr_len + (int)sizeof(struct icmphdr);
            if (received - inner_off >= (int)sizeof(struct iphdr) + (int)sizeof(struct icmphdr)) {
                struct iphdr *orig_ip = (struct iphdr *)(buffer + inner_off);
                int orig_ip_len = orig_ip->ihl * 4;
                struct icmphdr *orig_icmp =
                    (struct icmphdr *)(buffer + inner_off + orig_ip_len);
                if (ntohs(orig_icmp->un.echo.id) == config->pid) {
                    char from_ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &from.sin_addr, from_ip, sizeof(from_ip));
                    print_error(config, from_ip, icmp_hdr->type, icmp_hdr->code);
                    return 1;
                }
            }
            continue;
        }

        // Ignorer les paquets qui ne sont pas pour nous
        if (icmp_hdr->type != ICMP_ECHOREPLY ||
            ntohs(icmp_hdr->un.echo.id) != config->pid ||
            ntohs(icmp_hdr->un.echo.sequence) != config->seq) {
            continue;
        }

        // C'est notre réponse !
        double rtt = calculate_rtt(start, &end);
        update_stats(stats, rtt);
        print_reply(config, received - ip_hdr_len, rtt, ip_hdr->ttl);

        stats->received++;
        return 0;
    }

    return -1;
}