#include "ft_ping.h"

int main(int argc, char **argv) {
    t_ping_config   config;
    t_ping_stats    stats;

    memset(&config, 0, sizeof(config));
    config.pid = getpid() & 0xFFFF;

    if (parse_args(argc, argv, &config) != 0) {
        return 1;
    }

    if (resolve_hostname(&config) != 0) {
        return 1;
    }

    if (create_socket(&config) < 0) {
        perror("Socket creation failed");
        return 1;
    }

    setup_signals();

    init_stats(&stats);
    gettimeofday(&stats.start_time, NULL);

    printf("PING %s (%s): %d data bytes\n", config.hostname,
           config.resolved_ip, DATA_SIZE, PACKET_SIZE);

    while (g_running) {
        struct timeval start;
        gettimeofday(&start, NULL);

        if (send_ping(&config, &stats) < 0)
            break;
        
        if (receive_ping(&config, &stats, &start) == 0)
        {}
        else {
            if (config.verbose)
                printf("Request timeout for icmp_seq %d\n", config.seq);
        }
        config.seq++;
        sleep(1);
    }

    print_stats(&config, &stats);
    close(config.sockfd);
    return 0;
}