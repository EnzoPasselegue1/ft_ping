#include "ft_ping.h"
#include <string.h>
#include <unistd.h>

// Couleurs pour l'affichage
#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define BLUE "\033[0;34m"
#define YELLOW "\033[0;33m"
#define RESET "\033[0m"

// Helper pour créer une config complète
t_ping_config create_full_config(const char *hostname) {
    t_ping_config config;
    memset(&config, 0, sizeof(t_ping_config));
    
    config.hostname = (char *)hostname;
    config.pid = getpid() & 0xFFFF;  // 16 bits pour ICMP
    config.seq = 1;
    config.verbose = 0;
    
    return config;
}

// Helper pour créer des stats vierges
t_ping_stats create_clean_stats(void) {
    t_ping_stats stats;
    memset(&stats, 0, sizeof(t_ping_stats));
    stats.min_rtt = -1;
    stats.max_rtt = 0;
    return stats;
}

// Helper pour afficher les stats
void print_stats_summary(t_ping_stats *stats) {
    printf("  Statistiques:\n");
    printf("    Transmis: %d\n", stats->transmitted);
    printf("    Reçus: %d\n", stats->received);
    
    if (stats->transmitted > 0) {
        double loss = ((stats->transmitted - stats->received) * 100.0) / 
                      stats->transmitted;
        printf("    Perte: %.1f%%\n", loss);
    }
    
    if (stats->received > 0) {
        double avg = stats->sum_rtt / stats->received;
        printf("    RTT min: %.3f ms\n", stats->min_rtt);
        printf("    RTT max: %.3f ms\n", stats->max_rtt);
        printf("    RTT moy: %.3f ms\n", avg);
        
        double variance = (stats->sum_rtt_sq / stats->received) - (avg * avg);
        double mdev = variance > 0 ? sqrt(variance) : 0;
        printf("    RTT mdev: %.3f ms\n", mdev);
    }
}

// Helper pour vérifier les permissions
int check_permissions(void) {
    int test_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (test_sock < 0) {
        return 0;
    }
    close(test_sock);
    return 1;
}

void test_send_basic(void) {
    printf(BLUE "Test 1: Envoi basique d'un paquet\n" RESET);
    
    if (!check_permissions()) {
        printf(YELLOW "⚠ SKIP: Nécessite sudo\n" RESET);
        printf("\n");
        return;
    }
    
    t_ping_config config = create_full_config("127.0.0.1");
    t_ping_stats stats = create_clean_stats();
    
    if (resolve_hostname(&config) != 0 || create_socket(&config) != 0) {
        printf(RED "✗ FAIL: Setup échoué\n" RESET);
        printf("\n");
        return;
    }
    
    int result = send_ping(&config, &stats);
    
    if (result == 0 && stats.transmitted == 1) {
        printf(GREEN "✓ PASS: Paquet envoyé\n" RESET);
        printf("  PID: %d, Seq: %d\n", config.pid, config.seq);
    } else {
        printf(RED "✗ FAIL: Échec d'envoi (result=%d, transmitted=%d)\n" RESET, 
               result, stats.transmitted);
    }
    
    close(config.sockfd);
    printf("\n");
}

void test_send_increment_stats(void) {
    printf(BLUE "Test 2: Incrémentation des stats à chaque envoi\n" RESET);
    
    if (!check_permissions()) {
        printf(YELLOW "⚠ SKIP: Nécessite sudo\n" RESET);
        printf("\n");
        return;
    }
    
    t_ping_config config = create_full_config("127.0.0.1");
    t_ping_stats stats = create_clean_stats();
    
    if (resolve_hostname(&config) != 0 || create_socket(&config) != 0) {
        printf(RED "✗ FAIL: Setup échoué\n" RESET);
        printf("\n");
        return;
    }
    
    int count = 5;
    int success = 0;
    
    for (int i = 0; i < count; i++) {
        config.seq = i + 1;
        if (send_ping(&config, &stats) == 0) {
            success++;
        }
    }
    
    if (success == count && stats.transmitted == count) {
        printf(GREEN "✓ PASS: Stats correctement incrémentées\n" RESET);
        printf("  Envoyés: %d (attendu: %d)\n", stats.transmitted, count);
    } else {
        printf(RED "✗ FAIL: Stats incorrectes\n" RESET);
        printf("  Envoyés: %d (attendu: %d)\n", stats.transmitted, count);
    }
    
    close(config.sockfd);
    printf("\n");
}

void test_receive_localhost(void) {
    printf(BLUE "Test 3: Réception sur localhost\n" RESET);
    
    if (!check_permissions()) {
        printf(YELLOW "⚠ SKIP: Nécessite sudo\n" RESET);
        printf("\n");
        return;
    }
    
    t_ping_config config = create_full_config("127.0.0.1");
    t_ping_stats stats = create_clean_stats();
    
    if (resolve_hostname(&config) != 0 || create_socket(&config) != 0) {
        printf(RED "✗ FAIL: Setup échoué\n" RESET);
        printf("\n");
        return;
    }
    
    struct timeval start;
    gettimeofday(&start, NULL);
    
    // Envoyer
    if (send_ping(&config, &stats) != 0) {
        printf(RED "✗ FAIL: Envoi échoué\n" RESET);
        close(config.sockfd);
        printf("\n");
        return;
    }
    
    printf("  Paquet envoyé, attente réponse...\n");
    
    // Recevoir
    int result = receive_ping(&config, &stats, &start);
    
    if (result == 0 && stats.received == 1) {
        printf(GREEN "✓ PASS: Réponse reçue et stats mises à jour\n" RESET);
        print_stats_summary(&stats);
    } else {
        printf(RED "✗ FAIL: Pas de réponse (result=%d, received=%d)\n" RESET, 
               result, stats.received);
    }
    
    close(config.sockfd);
    printf("\n");
}

void test_ping_sequence(void) {
    printf(BLUE "Test 4: Séquence complète de 3 pings\n" RESET);
    
    if (!check_permissions()) {
        printf(YELLOW "⚠ SKIP: Nécessite sudo\n" RESET);
        printf("\n");
        return;
    }
    
    t_ping_config config = create_full_config("127.0.0.1");
    t_ping_stats stats = create_clean_stats();
    config.verbose = 1;  // Activer verbose pour voir les réponses
    
    if (resolve_hostname(&config) != 0 || create_socket(&config) != 0) {
        printf(RED "✗ FAIL: Setup échoué\n" RESET);
        printf("\n");
        return;
    }
    
    printf("\n");
    for (int i = 0; i < 3; i++) {
        config.seq = i + 1;
        struct timeval start;
        gettimeofday(&start, NULL);
        
        send_ping(&config, &stats);
        receive_ping(&config, &stats, &start);
        
        sleep(1);
    }
    
    printf("\n");
    if (stats.transmitted == 3 && stats.received >= 2) {
        printf(GREEN "✓ PASS: Séquence réussie\n" RESET);
        print_stats_summary(&stats);
    } else {
        printf(YELLOW "⚠ PARTIAL: Quelques pertes\n" RESET);
        print_stats_summary(&stats);
    }
    
    close(config.sockfd);
    printf("\n");
}

void test_timeout_handling(void) {
    printf(BLUE "Test 5: Gestion du timeout\n" RESET);
    
    if (!check_permissions()) {
        printf(YELLOW "⚠ SKIP: Nécessite sudo\n" RESET);
        printf("\n");
        return;
    }
    
    // IP qui ne répond pas (TEST-NET-1 - RFC 5737)
    t_ping_config config = create_full_config("192.0.2.1");
    t_ping_stats stats = create_clean_stats();
    
    if (resolve_hostname(&config) != 0 || create_socket(&config) != 0) {
        printf(RED "✗ FAIL: Setup échoué\n" RESET);
        printf("\n");
        return;
    }
    
    printf("  Envoi vers IP injoignable (timeout attendu)...\n");
    
    struct timeval start;
    gettimeofday(&start, NULL);
    
    send_ping(&config, &stats);
    
    time_t before = time(NULL);
    int result = receive_ping(&config, &stats, &start);
    time_t elapsed = time(NULL) - before;
    
    printf("  Temps écoulé: %ld seconde(s)\n", elapsed);
    printf("  Code retour: %d\n", result);
    printf("  errno: %d (%s)\n", errno, strerror(errno));
    
    if (result == -1 && (errno == EAGAIN || errno == EWOULDBLOCK) && 
        elapsed >= 1) {
        printf(GREEN "✓ PASS: Timeout géré (EAGAIN/EWOULDBLOCK)\n" RESET);
    } else {
        printf(YELLOW "⚠ INFO: Vérifier le timeout\n" RESET);
    }
    
    close(config.sockfd);
    printf("\n");
}

void test_packet_validation(void) {
    printf(BLUE "Test 6: Validation PID et séquence\n" RESET);
    
    if (!check_permissions()) {
        printf(YELLOW "⚠ SKIP: Nécessite sudo\n" RESET);
        printf("\n");
        return;
    }
    
    t_ping_config config = create_full_config("127.0.0.1");
    t_ping_stats stats = create_clean_stats();
    
    if (resolve_hostname(&config) != 0 || create_socket(&config) != 0) {
        printf(RED "✗ FAIL: Setup échoué\n" RESET);
        printf("\n");
        return;
    }
    
    // Test avec PID et séquence spécifiques
    config.pid = 12345;
    config.seq = 42;
    
    struct timeval start;
    gettimeofday(&start, NULL);
    
    send_ping(&config, &stats);
    int result = receive_ping(&config, &stats, &start);
    
    if (result == 0) {
        printf(GREEN "✓ PASS: Paquet avec bon PID/seq accepté\n" RESET);
        printf("  PID: %d, Seq: %d\n", config.pid, config.seq);
    } else {
        printf(YELLOW "⚠ INFO: Vérifier validation PID/seq\n" RESET);
    }
    
    close(config.sockfd);
    printf("\n");
}

void test_rtt_calculation(void) {
    printf(BLUE "Test 7: Calcul du RTT et mise à jour stats\n" RESET);
    
    if (!check_permissions()) {
        printf(YELLOW "⚠ SKIP: Nécessite sudo\n" RESET);
        printf("\n");
        return;
    }
    
    t_ping_config config = create_full_config("127.0.0.1");
    t_ping_stats stats = create_clean_stats();
    
    if (resolve_hostname(&config) != 0 || create_socket(&config) != 0) {
        printf(RED "✗ FAIL: Setup échoué\n" RESET);
        printf("\n");
        return;
    }
    
    printf("  Collecte de 10 RTT...\n");
    
    for (int i = 0; i < 10; i++) {
        config.seq = i + 1;
        struct timeval start;
        gettimeofday(&start, NULL);
        
        send_ping(&config, &stats);
        receive_ping(&config, &stats, &start);
        
        usleep(100000); // 100ms
    }
    
    printf("\n");
    if (stats.received >= 8) {
        printf(GREEN "✓ PASS: Statistiques RTT collectées\n" RESET);
        print_stats_summary(&stats);
        
        // Vérifications de cohérence
        int coherent = 1;
        
        if (stats.min_rtt < 0 || stats.min_rtt > stats.max_rtt) {
            printf(RED "  ✗ Incohérence min/max\n" RESET);
            coherent = 0;
        }
        
        double avg = stats.sum_rtt / stats.received;
        if (avg < stats.min_rtt || avg > stats.max_rtt) {
            printf(RED "  ✗ Moyenne hors intervalle\n" RESET);
            coherent = 0;
        }
        
        if (stats.sum_rtt_sq < 0) {
            printf(RED "  ✗ Somme des carrés négative\n" RESET);
            coherent = 0;
        }
        
        if (coherent) {
            printf(GREEN "  ✓ Toutes les stats sont cohérentes\n" RESET);
        }
    } else {
        printf(RED "✗ FAIL: Trop de pertes (%d/10)\n" RESET, stats.received);
    }
    
    close(config.sockfd);
    printf("\n");
}

void test_invalid_socket(void) {
    printf(BLUE "Test 8: Gestion socket invalide\n" RESET);
    
    t_ping_config config = create_full_config("127.0.0.1");
    t_ping_stats stats = create_clean_stats();
    struct timeval start;
    
    config.sockfd = -1;  // Socket invalide
    gettimeofday(&start, NULL);
    
    int send_result = send_ping(&config, &stats);
    int recv_result = receive_ping(&config, &stats, &start);
    
    if (send_result == -1 && recv_result == -1) {
        printf(GREEN "✓ PASS: Socket invalide détecté\n" RESET);
    } else {
        printf(RED "✗ FAIL: Devrait échouer avec socket invalide\n" RESET);
    }
    printf("\n");
}

void test_verbose_mode(void) {
    printf(BLUE "Test 9: Mode verbose\n" RESET);
    
    if (!check_permissions()) {
        printf(YELLOW "⚠ SKIP: Nécessite sudo\n" RESET);
        printf("\n");
        return;
    }
    
    t_ping_config config = create_full_config("127.0.0.1");
    t_ping_stats stats = create_clean_stats();
    config.verbose = 1;
    
    if (resolve_hostname(&config) != 0 || create_socket(&config) != 0) {
        printf(RED "✗ FAIL: Setup échoué\n" RESET);
        printf("\n");
        return;
    }
    
    printf("  Mode verbose activé, envoi d'un ping:\n\n");
    
    struct timeval start;
    gettimeofday(&start, NULL);
    
    send_ping(&config, &stats);
    receive_ping(&config, &stats, &start);
    
    printf("\n");
    printf(GREEN "✓ INFO: Vérifier que print_reply() a été appelé ci-dessus\n" RESET);
    
    close(config.sockfd);
    printf("\n");
}

void test_multiple_packets_loss(void) {
    printf(BLUE "Test 10: Calcul du taux de perte\n" RESET);
    
    if (!check_permissions()) {
        printf(YELLOW "⚠ SKIP: Nécessite sudo\n" RESET);
        printf("\n");
        return;
    }
    
    t_ping_config config = create_full_config("127.0.0.1");
    t_ping_stats stats = create_clean_stats();
    
    if (resolve_hostname(&config) != 0 || create_socket(&config) != 0) {
        printf(RED "✗ FAIL: Setup échoué\n" RESET);
        printf("\n");
        return;
    }
    
    int total = 20;
    for (int i = 0; i < total; i++) {
        config.seq = i + 1;
        struct timeval start;
        gettimeofday(&start, NULL);
        
        send_ping(&config, &stats);
        receive_ping(&config, &stats, &start);
        
        usleep(50000); // 50ms
    }
    
    printf("\n");
    double loss = ((stats.transmitted - stats.received) * 100.0) / 
                  stats.transmitted;
    
    if (loss <= 10.0) {  // Moins de 10% de perte
        printf(GREEN "✓ PASS: Taux de perte acceptable (%.1f%%)\n" RESET, loss);
        print_stats_summary(&stats);
    } else {
        printf(YELLOW "⚠ WARNING: Taux de perte élevé (%.1f%%)\n" RESET, loss);
        print_stats_summary(&stats);
    }
    
    close(config.sockfd);
    printf("\n");
}

int main(void) {
    printf("\n");
    printf("================================================\n");
    printf("   TESTS POUR send_ping() / receive_ping()\n");
    printf("================================================\n");
    
    if (!check_permissions()) {
        printf(YELLOW "\n⚠ AVERTISSEMENT:\n" RESET);
        printf("Ces tests nécessitent les permissions root.\n");
        printf("Relancez avec: sudo ./test_ping\n\n");
    } else {
        printf(GREEN "\n✓ Permissions root détectées\n" RESET);
        printf("\n");
    }

    test_send_basic();
    test_send_increment_stats();
    test_receive_localhost();
    test_packet_validation();
    test_timeout_handling();
    test_rtt_calculation();
    test_ping_sequence();
    test_multiple_packets_loss();
    test_verbose_mode();
    test_invalid_socket();
    
    printf("================================================\n");
    printf("              FIN DES TESTS\n");
    printf("================================================\n\n");
    
    printf(YELLOW "Notes importantes:\n" RESET);
    printf("  - Nécessite sudo pour RAW sockets\n");
    printf("  - Timeout configuré à 1 sec dans create_socket()\n");
    printf("  - RTT sur localhost généralement < 1ms\n");
    printf("  - Vérifier firewall si échecs multiples\n");
    printf("  - Les fonctions helpers attendues:\n");
    printf("    • calculate_rtt()\n");
    printf("    • update_stats()\n");
    printf("    • print_reply()\n");
    printf("    • print_error() (mode verbose)\n\n");

    return 0;
}