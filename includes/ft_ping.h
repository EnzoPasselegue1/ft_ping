#ifndef FT_PING_H
# define FT_PING_H

# include<stdio.h>
# include<stdlib.h>
# include<string.h>
# include<unistd.h>
# include<signal.h>
# include<errno.h>
#include <getopt.h>
# include<sys/types.h>
# include<sys/socket.h>
# include<sys/time.h>
# include<netinet/in.h>
# include<netinet/ip.h>
# include<netinet/ip_icmp.h>
# include<arpa/inet.h>
# include<netdb.h>
# include<math.h>

# define PACKET_SIZE 64
# define DATA_SIZE 56

// Structures
typedef struct s_ping_config {
    char            *hostname;
    char            resolved_ip[INET_ADDRSTRLEN];
    int             verbose;
    int             sockfd;
    uint16_t        pid;
    uint16_t        seq;
    struct sockaddr_in dest_addr;
} t_ping_config;

typedef struct s_ping_stats {
    int             transmitted;
    int             received;
    double          min_rtt;
    double          max_rtt;
    double          sum_rtt;
    double          sum_rtt_sq;
    struct timeval  start_time;
} t_ping_stats;

typedef struct s_icmp_packet {
    struct icmphdr  header;
    char            data[DATA_SIZE];
} t_icmp_packet;

// Prototypes
// parser.c
int     parse_args(int argc, char **argv, t_ping_config *config);
void    print_help(void);

// dns.c
int     resolve_hostname(t_ping_config *config);

// socket.c
int     create_socket(t_ping_config *config);

// icmp.c
void    build_icmp_packet(t_icmp_packet *packet, t_ping_config *config);
uint16_t calculate_checksum(void *data, int len);

// send_receive.c
int     send_ping(t_ping_config *config, t_ping_stats *stats);
int     receive_ping(t_ping_config *config, t_ping_stats *stats, struct timeval *start);

// timing.c
double  calculate_rtt(struct timeval *start, struct timeval *end);
double  timeval_to_ms(struct timeval *tv);

// stats.c
void    init_stats(t_ping_stats *stats);
void    update_stats(t_ping_stats *stats, double rtt);
void    print_stats(t_ping_config *config, t_ping_stats *stats);

// signal.c
void    setup_signals(void);

// display.c
void    print_reply(t_ping_config *config, int bytes, double rtt, int ttl);
void    print_error(t_ping_config *config, int type, int code);

// Global
extern volatile sig_atomic_t g_running;

#endif