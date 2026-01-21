#include "ft_ping.h"

extern volatile sig_atomic_t g_running;

void handle_sigint(int signum) {
    (void)signum;
    g_running = 0;
}

void setup_signals(void) 
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    // Gérer SIGINT (Ctrl+C)
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("ft_ping: sigaction SIGINT");
        exit(EXIT_FAILURE);
    }

    // Gérer SIGALRM (timeout du script)
    if (sigaction(SIGALRM, &sa, NULL) < 0) {
        perror("ft_ping: sigaction SIGALRM");
        exit(EXIT_FAILURE);
    }
}