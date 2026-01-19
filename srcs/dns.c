#include "ft_ping.h"

int     resolve_hostname(t_ping_config *config)
{
    struct addrinfo hints, *result;
    int ret;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_RAW;
    hints.ai_protocol = IPPROTO_ICMP;

    ret = getaddrinfo(config->hostname, NULL, &hints, &result);

    if (ret != 0) {
        fprintf(stderr, "ft_ping: %s %s\n", config->hostname, gai_strerror(ret));
        return -1;
    }
    
    struct sockaddr_in *addr = (struct sockaddr_in *)result->ai_addr;
    memcpy(&config->dest_addr, addr, sizeof(struct sockaddr_in));
    inet_ntop(AF_INET, &addr->sin_addr, config->resolved_ip, INET_ADDRSTRLEN);
    freeaddrinfo(result);
    return 0;
}
