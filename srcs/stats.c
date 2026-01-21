#include "ft_ping.h"

void    init_stats(t_ping_stats *stats)
{
    memset(stats, 0, sizeof(t_ping_stats));
    stats->min_rtt = 999999.0;
    stats->max_rtt = 0.0;
}

void update_stats(t_ping_stats *stats, double rtt) {
    if (rtt < stats->min_rtt)
        stats->min_rtt = rtt;
    if (rtt > stats->max_rtt)
        stats->max_rtt = rtt;

    stats->sum_rtt += rtt;
    stats->sum_rtt_sq += rtt * rtt;
}

void    print_stats(t_ping_config *config, t_ping_stats *stats)
{
    struct timeval end;
    gettimeofday(&end, NULL);

    double total_time = calculate_rtt(&stats->start_time, &end);
    double loss_percent = 100.0 * (stats->transmitted - stats->received) / stats->transmitted;

    printf("\n---%s ping statistics ---\n", config->hostname);
    printf("%d packets transmitted,%d received,%.0f%% packet loss, time%.0fms\n", stats->transmitted, stats->received, loss_percent, total_time);

    if (stats->received > 0)
    {
        double avg = stats->sum_rtt / stats->received;
        double variance = (stats->sum_rtt_sq / stats->received) - (avg * avg);
        double stddev = sqrt(variance > 0 ? variance : 0);

        printf("rtt min/avg/max/mdev =%.3f/%.3f/%.3f/%.3f ms\n",
               stats->min_rtt, avg, stats->max_rtt, stddev);
    }
}
