#include "ft_ping.h"

volatile sig_atomic_t g_running = 1;

int main(int argc, char **argv) {
    t_ping_config   config;
    t_ping_stats    stats;

    memset(&config, 0, sizeof(config));
    config.pid = getpid() & 0xFFFF;
    config.seq = 1;  // Important !

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

    printf("PING %s (%s) %d(%d) bytes of data.\n",
           config.hostname,
           config.resolved_ip,
           DATA_SIZE,
           PACKET_SIZE + IP_HEADER_SIZE);

    // Like the reference ping, there is no default packet count: we keep
    // sending until the user interrupts us with CTRL+C (g_running is
    // cleared by the SIGINT handler in signal.c).
    while (g_running) {
        struct timeval start;
        gettimeofday(&start, NULL);

        if (send_ping(&config, &stats) < 0)
            break;

        if (receive_ping(&config, &stats, &start) < 0) {
            if (config.verbose)
                printf("Request timeout for icmp_seq %d\n", config.seq);
        }

        config.seq++;

        if (g_running)
            sleep(1);
    }

    print_stats(&config, &stats);
    close(config.sockfd);
    return 0;
}