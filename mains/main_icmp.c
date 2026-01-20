#include "ft_ping.h"
#include <string.h>
#include <time.h>
#include "unistd.h"

// Couleurs pour l'affichage
#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define BLUE "\033[0;34m"
#define YELLOW "\033[0;33m"
#define RESET "\033[0m"

// Helper pour créer une config vierge
t_ping_config create_clean_config(void) {
    t_ping_config config;
    memset(&config, 0, sizeof(t_ping_config));
    config.pid = getpid();
    config.seq = 1;
    return config;
}

// Helper pour afficher un paquet en hexadécimal
void print_packet_hex(t_icmp_packet *packet, int bytes) {
    uint8_t *ptr = (uint8_t *)packet;
    printf("  Hex dump (%d bytes):\n  ", bytes);
    for (int i = 0; i < bytes; i++) {
        printf("%02x ", ptr[i]);
        if ((i + 1) % 16 == 0)
            printf("\n  ");
    }
    printf("\n");
}

// Helper pour afficher les détails du header ICMP
void print_icmp_header(struct icmphdr *header) {
    printf("  ICMP Header:\n");
    printf("    Type: %u (ICMP_ECHO = %u)\n", header->type, ICMP_ECHO);
    printf("    Code: %u\n", header->code);
    printf("    Checksum: 0x%04x\n", ntohs(header->checksum));
    printf("    ID: %u (host order: %u)\n", ntohs(header->un.echo.id), header->un.echo.id);
    printf("    Sequence: %u (host order: %u)\n", ntohs(header->un.echo.sequence), header->un.echo.sequence);
}

// Helper pour vérifier le checksum
int verify_checksum(t_icmp_packet *packet) {
    uint16_t original = packet->header.checksum;
    packet->header.checksum = 0;
    uint16_t calculated = calculate_checksum(packet, sizeof(t_icmp_packet));
    packet->header.checksum = original;
    return (original == calculated);
}

void test_checksum_basic(void) {
    printf(BLUE "Test 1: Calcul de checksum basique\n" RESET);
    
    // Données de test simples
    uint8_t data[] = {0x45, 0x00, 0x00, 0x54, 0x00, 0x00, 0x40, 0x00};
    uint16_t checksum = calculate_checksum(data, sizeof(data));
    
    printf("  Données: ");
    for (size_t i = 0; i < sizeof(data); i++)
        printf("%02x ", data[i]);
    printf("\n");
    printf("  Checksum calculé: 0x%04x\n", checksum);
    
    // Le checksum ne devrait pas être 0 (sauf cas très rare)
    if (checksum != 0) {
        printf(GREEN "✓ PASS: Checksum calculé (non-zéro)\n" RESET);
    } else {
        printf(YELLOW "⚠ INFO: Checksum est 0 (rare mais possible)\n" RESET);
    }
    printf("\n");
}

void test_checksum_zeros(void) {
    printf(BLUE "Test 2: Checksum de données nulles\n" RESET);
    
    uint8_t zeros[64];
    memset(zeros, 0, sizeof(zeros));
    
    uint16_t checksum = calculate_checksum(zeros, sizeof(zeros));
    
    printf("  Taille: %zu bytes de zéros\n", sizeof(zeros));
    printf("  Checksum: 0x%04x\n", checksum);
    
    // Pour des zéros, le checksum devrait être 0xFFFF
    if (checksum == 0xFFFF) {
        printf(GREEN "✓ PASS: Checksum correct pour zéros (0xFFFF)\n" RESET);
    } else {
        printf(RED "✗ FAIL: Attendu 0xFFFF, obtenu 0x%04x\n" RESET, checksum);
    }
    printf("\n");
}

void test_checksum_odd_length(void) {
    printf(BLUE "Test 3: Checksum avec longueur impaire\n" RESET);
    
    // Test avec 7 bytes (impair)
    uint8_t data[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE};
    uint16_t checksum = calculate_checksum(data, sizeof(data));
    
    printf("  Longueur impaire: %zu bytes\n", sizeof(data));
    printf("  Checksum: 0x%04x\n", checksum);
    
    if (checksum != 0) {
        printf(GREEN "✓ PASS: Gestion longueur impaire OK\n" RESET);
    } else {
        printf(YELLOW "⚠ INFO: Checksum est 0\n" RESET);
    }
    printf("\n");
}

void test_checksum_verification(void) {
    printf(BLUE "Test 4: Vérification checksum (propriété d'inversion)\n" RESET);
    
    uint8_t data[] = {0x45, 0x00, 0x00, 0x3c, 0x1c, 0x46, 0x40, 0x00,
                      0x40, 0x06, 0x00, 0x00, 0xac, 0x10, 0x0a, 0x63};
    
    uint16_t checksum1 = calculate_checksum(data, sizeof(data));
    
    // Insérer le checksum dans les données
    data[10] = (checksum1 >> 8) & 0xFF;
    data[11] = checksum1 & 0xFF;
    
    // Recalculer - devrait donner 0
    uint16_t checksum2 = calculate_checksum(data, sizeof(data));
    
    printf("  Checksum original: 0x%04x\n", checksum1);
    printf("  Checksum après insertion: 0x%04x\n", checksum2);
    
    if (checksum2 == 0) {
        printf(GREEN "✓ PASS: Propriété de vérification OK\n" RESET);
    } else {
        printf(RED "✗ FAIL: Devrait être 0 après insertion\n" RESET);
    }
    printf("\n");
}

void test_packet_basic_structure(void) {
    printf(BLUE "Test 5: Structure basique du paquet ICMP\n" RESET);
    
    t_ping_config config = create_clean_config();
    t_icmp_packet packet;
    
    build_icmp_packet(&packet, &config);
    
    print_icmp_header(&packet.header);
    
    int pass = 1;
    
    if (packet.header.type != ICMP_ECHO) {
        printf(RED "  ✗ Type incorrect: %u (attendu %u)\n" RESET, 
               packet.header.type, ICMP_ECHO);
        pass = 0;
    }
    
    if (packet.header.code != 0) {
        printf(RED "  ✗ Code incorrect: %u (attendu 0)\n" RESET, packet.header.code);
        pass = 0;
    }
    
    if (ntohs(packet.header.un.echo.id) != config.pid) {
        printf(RED "  ✗ ID incorrect: %u (attendu %u)\n" RESET, 
               ntohs(packet.header.un.echo.id), config.pid);
        pass = 0;
    }
    
    if (ntohs(packet.header.un.echo.sequence) != config.seq) {
        printf(RED "  ✗ Sequence incorrecte: %u (attendu %u)\n" RESET, 
               ntohs(packet.header.un.echo.sequence), config.seq);
        pass = 0;
    }
    
    if (pass) {
        printf(GREEN "✓ PASS: Structure ICMP correcte\n" RESET);
    } else {
        printf(RED "✗ FAIL: Erreurs dans la structure\n" RESET);
    }
    printf("\n");
}

void test_packet_checksum_validity(void) {
    printf(BLUE "Test 6: Validité du checksum du paquet\n" RESET);
    
    t_ping_config config = create_clean_config();
    t_icmp_packet packet;
    
    build_icmp_packet(&packet, &config);
    
    printf("  Checksum dans le paquet: 0x%04x\n", ntohs(packet.header.checksum));
    
    if (verify_checksum(&packet)) {
        printf(GREEN "✓ PASS: Checksum valide\n" RESET);
    } else {
        printf(RED "✗ FAIL: Checksum invalide\n" RESET);
        
        // Afficher le checksum recalculé pour debug
        uint16_t saved = packet.header.checksum;
        packet.header.checksum = 0;
        uint16_t recalc = calculate_checksum(&packet, sizeof(t_icmp_packet));
        printf("  Checksum recalculé: 0x%04x\n", recalc);
        printf("  Checksum original: 0x%04x\n", saved);
        packet.header.checksum = saved;
    }
    printf("\n");
}

void test_packet_timestamp(void) {
    printf(BLUE "Test 7: Présence du timestamp dans les données\n" RESET);
    
    t_ping_config config = create_clean_config();
    t_icmp_packet packet;
    
    struct timeval before, after, *embedded;
    gettimeofday(&before, NULL);
    build_icmp_packet(&packet, &config);
    gettimeofday(&after, NULL);
    
    embedded = (struct timeval *)packet.data;
    
    printf("  Timestamp avant: %ld.%06ld\n", before.tv_sec, before.tv_usec);
    printf("  Timestamp packet: %ld.%06ld\n", embedded->tv_sec, embedded->tv_usec);
    printf("  Timestamp après: %ld.%06ld\n", after.tv_sec, after.tv_usec);
    
    // Vérifier que le timestamp est dans l'intervalle
    if (embedded->tv_sec >= before.tv_sec && embedded->tv_sec <= after.tv_sec) {
        printf(GREEN "✓ PASS: Timestamp valide\n" RESET);
    } else {
        printf(RED "✗ FAIL: Timestamp hors intervalle\n" RESET);
    }
    printf("\n");
}

void test_packet_data_pattern(void) {
    printf(BLUE "Test 8: Motif de remplissage des données\n" RESET);
    
    t_ping_config config = create_clean_config();
    t_icmp_packet packet;
    
    build_icmp_packet(&packet, &config);
    
    // Vérifier le motif après le timestamp
    int pattern_ok = 1;
    for (int i = sizeof(struct timeval); i < DATA_SIZE; i++) {
        if (packet.data[i] != (uint8_t)i) {
            printf(RED "  ✗ Erreur à data[%d]: attendu %d, obtenu %d\n" RESET, 
                   i, i, packet.data[i]);
            pattern_ok = 0;
            break;
        }
    }
    
    if (pattern_ok) {
        printf(GREEN "✓ PASS: Motif de données correct\n" RESET);
        printf("  Premiers bytes après timestamp: ");
        for (int i = sizeof(struct timeval); i < sizeof(struct timeval) + 8 && i < DATA_SIZE; i++)
            printf("%02x ", packet.data[i]);
        printf("\n");
    } else {
        printf(RED "✗ FAIL: Motif de données incorrect\n" RESET);
    }
    printf("\n");
}

void test_packet_sequence_increment(void) {
    printf(BLUE "Test 9: Incrémentation de la séquence\n" RESET);
    
    t_ping_config config = create_clean_config();
    t_icmp_packet packets[5];
    
    int pass = 1;
    for (int i = 0; i < 5; i++) {
        config.seq = i + 1;
        build_icmp_packet(&packets[i], &config);
        
        uint16_t seq = ntohs(packets[i].header.un.echo.sequence);
        printf("  Paquet %d - Séquence: %u\n", i + 1, seq);
        
        if (seq != config.seq) {
            printf(RED "  ✗ Attendu: %u, Obtenu: %u\n" RESET, config.seq, seq);
            pass = 0;
        }
    }
    
    if (pass) {
        printf(GREEN "✓ PASS: Séquences correctes\n" RESET);
    } else {
        printf(RED "✗ FAIL: Erreurs de séquence\n" RESET);
    }
    printf("\n");
}

void test_packet_different_pids(void) {
    printf(BLUE "Test 10: Différents PIDs\n" RESET);
    
    uint16_t pids[] = {100, 1000, 10000, 65535};
    int pass = 1;
    
    for (size_t i = 0; i < sizeof(pids) / sizeof(pids[0]); i++) {
        t_ping_config config = create_clean_config();
        config.pid = pids[i];
        
        t_icmp_packet packet;
        build_icmp_packet(&packet, &config);
        
        uint16_t id = ntohs(packet.header.un.echo.id);
        printf("  PID: %u -> ID paquet: %u\n", pids[i], id);
        
        if (id != pids[i]) {
            printf(RED "  ✗ Mismatch!\n" RESET);
            pass = 0;
        }
    }
    
    if (pass) {
        printf(GREEN "✓ PASS: Tous les PIDs corrects\n" RESET);
    } else {
        printf(RED "✗ FAIL: Erreurs de PID\n" RESET);
    }
    printf("\n");
}

void test_packet_size(void) {
    printf(BLUE "Test 11: Taille du paquet\n" RESET);
    
    t_ping_config config = create_clean_config();
    t_icmp_packet packet;
    
    build_icmp_packet(&packet, &config);
    
    size_t expected_size = sizeof(struct icmphdr) + DATA_SIZE;
    size_t actual_size = sizeof(t_icmp_packet);
    
    printf("  Taille attendue: %zu bytes\n", expected_size);
    printf("  Taille réelle: %zu bytes\n", actual_size);
    printf("  Header: %zu bytes\n", sizeof(struct icmphdr));
    printf("  Data: %d bytes\n", DATA_SIZE);
    
    if (actual_size == expected_size) {
        printf(GREEN "✓ PASS: Taille correcte\n" RESET);
    } else {
        printf(RED "✗ FAIL: Taille incorrecte\n" RESET);
    }
    printf("\n");
}

void test_checksum_consistency(void) {
    printf(BLUE "Test 12: Cohérence du checksum sur paquets identiques\n" RESET);
    
    t_ping_config config = create_clean_config();
    config.seq = 42; // Séquence fixe
    
    // Créer deux paquets avec un délai pour avoir des timestamps différents
    t_icmp_packet packet1, packet2;
    build_icmp_packet(&packet1, &config);
    
    // Utiliser nanosleep au lieu de usleep
    struct timespec delay = {0, 1000000}; // 1ms (0 sec, 1000000 nanosec)
    nanosleep(&delay, NULL);
    
    build_icmp_packet(&packet2, &config);
    
    printf("  Checksum paquet 1: 0x%04x\n", ntohs(packet1.header.checksum));
    printf("  Checksum paquet 2: 0x%04x\n", ntohs(packet2.header.checksum));
    
    // Les checksums devraient être différents à cause du timestamp
    if (packet1.header.checksum != packet2.header.checksum) {
        printf(GREEN "✓ PASS: Checksums différents (timestamps différents)\n" RESET);
    } else {
        printf(YELLOW "⚠ INFO: Checksums identiques (rare mais possible)\n" RESET);
    }
    
    // Les deux checksums devraient être valides
    if (verify_checksum(&packet1) && verify_checksum(&packet2)) {
        printf(GREEN "✓ PASS: Les deux checksums sont valides\n" RESET);
    } else {
        printf(RED "✗ FAIL: Au moins un checksum invalide\n" RESET);
    }
    printf("\n");
}

void test_packet_hex_dump(void) {
    printf(BLUE "Test 13: Dump hexadécimal du paquet\n" RESET);
    
    t_ping_config config = create_clean_config();
    config.pid = 12345;
    config.seq = 1;
    
    t_icmp_packet packet;
    build_icmp_packet(&packet, &config);
    
    printf("  Paquet ICMP complet:\n");
    print_packet_hex(&packet, sizeof(t_icmp_packet));
    
    printf(GREEN "✓ INFO: Dump affiché (vérification visuelle)\n" RESET);
    printf("\n");
}

int main(void) {
    printf("\n");
    printf("==========================================\n");
    printf("   TESTS POUR ICMP PACKET BUILDER\n");
    printf("==========================================\n\n");

    printf(YELLOW "Tests du calcul de checksum:\n" RESET);
    test_checksum_basic();
    test_checksum_zeros();
    test_checksum_odd_length();
    test_checksum_verification();
    
    printf(YELLOW "Tests de construction de paquets:\n" RESET);
    test_packet_basic_structure();
    test_packet_checksum_validity();
    test_packet_timestamp();
    test_packet_data_pattern();
    test_packet_size();
    
    printf(YELLOW "Tests de variations:\n" RESET);
    test_packet_sequence_increment();
    test_packet_different_pids();
    test_checksum_consistency();
    
    printf(YELLOW "Test visuel:\n" RESET);
    test_packet_hex_dump();
    
    printf("==========================================\n");
    printf("         FIN DES TESTS\n");
    printf("==========================================\n\n");
    
    printf(YELLOW "Notes:\n" RESET);
    printf("  - Le checksum ICMP utilise le complément à 1\n");
    printf("  - Les IDs et séquences sont en network byte order (big-endian)\n");
    printf("  - Le timestamp permet de calculer le RTT\n");
    printf("  - Le motif de données aide à détecter la corruption\n\n");

    return 0;
}