#include"ft_ping.h"
#include <linux/errqueue.h>

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

// Vide, sans bloquer, la file d'erreurs du socket alimentée par
// IP_RECVERR (voir socket.c). C'est le seul moyen de voir les erreurs que
// le noyau détecte lui-même AVANT d'émettre (ex: échec de résolution ARP
// sur le sous-réseau local) : elles ne passent jamais par un recvfrom()
// classique, contrairement aux erreurs distantes (Time Exceeded,
// Destination Unreachable) déjà gérées plus bas dans receive_ping().
//
// Cette file est propre à CE socket : tout ce qu'elle contient concerne
// forcément un envoi que nous avons fait nous-mêmes — pas besoin de
// revérifier l'id ICMP comme pour les erreurs distantes.
//
// Retourne 1 et remplit from_ip/type/code si une erreur a été trouvée,
// 0 si la file est vide pour l'instant.
static int poll_local_error(int sockfd, char *from_ip, size_t from_ip_len,
                             int *type, int *code) {
    char            buf[128];
    char            control[512];
    struct sockaddr_in from;
    struct msghdr   msg;
    struct iovec    iov = { buf, sizeof(buf) };
    struct cmsghdr  *cmsg;

    memset(&msg, 0, sizeof(msg));
    msg.msg_name = &from;
    msg.msg_namelen = sizeof(from);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = sizeof(control);

    if (recvmsg(sockfd, &msg, MSG_ERRQUEUE | MSG_DONTWAIT) < 0)
        return 0; // rien en attente (EAGAIN)

    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_level != IPPROTO_IP || cmsg->cmsg_type != IP_RECVERR)
            continue;

        struct sock_extended_err *ee = (struct sock_extended_err *)CMSG_DATA(cmsg);

        if (ee->ee_origin == SO_EE_ORIGIN_LOCAL) {
            // Le noyau a refusé d'émettre : on se rapporte nous-mêmes comme
            // source, comme le fait le ping de référence dans ce cas.
            struct sockaddr_in local;
            socklen_t len = sizeof(local);
            if (getsockname(sockfd, (struct sockaddr *)&local, &len) == 0)
                inet_ntop(AF_INET, &local.sin_addr, from_ip, from_ip_len);
            else
                snprintf(from_ip, from_ip_len, "0.0.0.0");

            *type = ICMP_DEST_UNREACH;
            *code = (ee->ee_errno == ENETUNREACH) ? ICMP_NET_UNREACH : ICMP_HOST_UNREACH;
            return 1;
        }
        if (ee->ee_origin == SO_EE_ORIGIN_ICMP) {
            // Erreur réseau réelle, remontée par ce second canal ; le
            // chemin recvfrom() normal la gère déjà, mais on la relaie
            // aussi ici si jamais elle n'apparaissait que par ce biais.
            struct sockaddr_in *offender = (struct sockaddr_in *)SO_EE_OFFENDER(ee);
            inet_ntop(AF_INET, &offender->sin_addr, from_ip, from_ip_len);
            *type = ee->ee_type;
            *code = ee->ee_code;
            return 1;
        }
    }
    return 0;
}

int receive_ping(t_ping_config *config, t_ping_stats *stats,
                 struct timeval *start) {
    char buffer[1024];
    struct sockaddr_in from;
    socklen_t fromlen;
    struct timeval deadline, now, end;
    int received;

    // Budget total identique à avant (SO_RCVTIMEO = 1s), mais découpé en
    // tranches via select() pour pouvoir revérifier la file d'erreurs
    // locales entre chaque tranche plutôt que de bloquer d'un bloc.
    gettimeofday(&deadline, NULL);
    deadline.tv_sec += 1;

    while (1) {
        char from_ip[INET_ADDRSTRLEN];
        int err_type, err_code;

        if (poll_local_error(config->sockfd, from_ip, sizeof(from_ip), &err_type, &err_code)) {
            print_error(config, from_ip, err_type, err_code);
            return 1;
        }

        gettimeofday(&now, NULL);
        long remaining_ms = (deadline.tv_sec - now.tv_sec) * 1000
                           + (deadline.tv_usec - now.tv_usec) / 1000;
        if (remaining_ms <= 0)
            return -1;

        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(config->sockfd, &rfds);
        long slice_ms = remaining_ms < 200 ? remaining_ms : 200;
        struct timeval tv;
        tv.tv_sec = slice_ms / 1000;
        tv.tv_usec = (slice_ms % 1000) * 1000;

        int ready = select(config->sockfd + 1, &rfds, NULL, NULL, &tv);
        if (ready < 0) {
            if (errno == EINTR)
                return -1; // CTRL+C pendant l'attente
            if (config->verbose)
                perror("ft_ping: select");
            return -1;
        }
        if (ready == 0)
            continue; // fin de tranche sans rien : on reboucle (re-vérifie la file d'erreurs, puis reprend l'attente)

        fromlen = sizeof(from);

        received = recvfrom(config->sockfd, buffer, sizeof(buffer), 0,
                            (struct sockaddr *)&from, &fromlen);

        if (received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
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

        double rtt = calculate_rtt(start, &end);
        update_stats(stats, rtt);
        print_reply(config, received - ip_hdr_len, rtt, ip_hdr->ttl);

        stats->received++;
        return 0;
    }
}
