#include "ft_ping.h"

void    print_help(void) {
    printf("Usage: ft_ping [OPTION...] HOST\n");
    printf("Send ICMP ECHO_REQUEST to network hosts.\n\n");
    printf("Options:\n");
    printf("  -v            Verbose output\n");
    printf("  -?            Show this help message\n");
    exit(0);
}

int     parse_args(int argc, char **argv, t_ping_config *config){
    int opt;

    if (argc < 2) {
        fprintf(stderr, "ft_ping: missing host operand\n");
        fprintf(stderr, "Usage: %s [OPTION...] HOST\n", argv[0]);
        return -1;
    }

    while ((opt = getopt(argc, argv, "v?")) != -1) {
        switch (opt) {
            case 'v':
                config->verbose = 1;
                break;
            case '?':
                print_help();
                break;
            default:
                fprintf(stderr, "Usage: %s [OPTION...] HOST\n", argv[0]);
                return -1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "ft_ping: missing host operand\n");
        fprintf(stderr, "Usage: %s [OPTION...] HOST\n", argv[0]);
        return -1;
    }

    config->hostname = argv[optind];
    return 0;
}