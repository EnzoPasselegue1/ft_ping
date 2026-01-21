#include"ft_ping.h"

double calculate_rtt(struct timeval *start, struct timeval *end) {
    double start_ms = start->tv_sec * 1000.0 + start->tv_usec / 1000.0;
    double end_ms = end->tv_sec * 1000.0 + end->tv_usec / 1000.0;
    return end_ms - start_ms;
}

double timeval_to_ms(struct timeval *tv) {
    return tv->tv_sec * 1000.0 + tv->tv_usec / 1000.0;
}