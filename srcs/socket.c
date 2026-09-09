#include "ft_ping.h"

int create_socket(t_ping_config *config) {
    struct timeval tv;

    config->sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (config->sockfd < 0) {
        if (errno == EPERM) {
            fprintf(stderr, "ft_ping: Lacking privilege for raw socket.\n");
            fprintf(stderr, "Try running with sudo.\n");
        } else {
            perror("ft_ping: socket");
        }
        return -1;
    }

    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if (setsockopt(config->sockfd, SOL_SOCKET, SO_RCVTIMEO,
                   &tv, sizeof(tv)) < 0) {
        perror("ft_ping: setsockopt");
        close(config->sockfd);
        return -1;
    }

    // Demande au noyau de nous remonter, via la file d'erreurs du socket
    // (recvmsg(..., MSG_ERRQUEUE) dans receive_ping), les échecs qu'il
    // détecte lui-même avant même d'émettre (ex: résolution ARP impossible
    // sur le sous-réseau local) — ces erreurs-là n'arrivent jamais par un
    // recvfrom() classique.
    int on = 1;
    if (setsockopt(config->sockfd, IPPROTO_IP, IP_RECVERR, &on, sizeof(on)) < 0) {
        perror("ft_ping: setsockopt(IP_RECVERR)");
        close(config->sockfd);
        return -1;
    }

    return 0;
}