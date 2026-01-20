#include "ft_ping.h"
#include <string.h>
#include <arpa/inet.h>

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
    return config;
}

// Helper pour vérifier si une IP est valide
int is_valid_ipv4(const char *ip) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip, &(sa.sin_addr)) == 1;
}

// Helper pour afficher les détails de la config
void print_config_details(t_ping_config *config) {
    printf("  Hostname: %s\n", config->hostname);
    printf("  Resolved IP: %s\n", config->resolved_ip);
    printf("  dest_addr.sin_family: %d (devrait être %d pour AF_INET)\n", 
           config->dest_addr.sin_family, AF_INET);
    
    char ip_from_struct[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &config->dest_addr.sin_addr, ip_from_struct, INET_ADDRSTRLEN);
    printf("  IP depuis dest_addr: %s\n", ip_from_struct);
}

void test_localhost(void) {
    printf(BLUE "Test 1: Résolution de 'localhost'\n" RESET);
    
    t_ping_config config = create_clean_config();
    config.hostname = "localhost";
    
    int result = resolve_hostname(&config);
    
    if (result == 0 && strcmp(config.resolved_ip, "127.0.0.1") == 0) {
        printf(GREEN "✓ PASS: localhost résolu en 127.0.0.1\n" RESET);
        print_config_details(&config);
    } else {
        printf(RED "✗ FAIL: Résolution de localhost incorrecte\n" RESET);
        if (result == 0) {
            print_config_details(&config);
        }
    }
    printf("\n");
}

void test_google_dns(void) {
    printf(BLUE "Test 2: Résolution de 'google.com'\n" RESET);
    
    t_ping_config config = create_clean_config();
    config.hostname = "google.com";
    
    int result = resolve_hostname(&config);
    
    if (result == 0 && is_valid_ipv4(config.resolved_ip)) {
        printf(GREEN "✓ PASS: google.com résolu avec succès\n" RESET);
        print_config_details(&config);
    } else {
        printf(RED "✗ FAIL: Échec résolution google.com\n" RESET);
    }
    printf("\n");
}

void test_direct_ip(void) {
    printf(BLUE "Test 3: Adresse IP directe (8.8.8.8)\n" RESET);
    
    t_ping_config config = create_clean_config();
    config.hostname = "8.8.8.8";
    
    int result = resolve_hostname(&config);
    
    if (result == 0 && strcmp(config.resolved_ip, "8.8.8.8") == 0) {
        printf(GREEN "✓ PASS: IP directe conservée\n" RESET);
        print_config_details(&config);
    } else {
        printf(RED "✗ FAIL: IP directe mal traitée\n" RESET);
        if (result == 0) {
            print_config_details(&config);
        }
    }
    printf("\n");
}

void test_invalid_hostname(void) {
    printf(BLUE "Test 4: Hostname invalide\n" RESET);
    
    t_ping_config config = create_clean_config();
    config.hostname = "this-hostname-definitely-does-not-exist-123456789.invalid";
    
    int result = resolve_hostname(&config);
    
    if (result == -1) {
        printf(GREEN "✓ PASS: Erreur détectée pour hostname invalide\n" RESET);
    } else {
        printf(RED "✗ FAIL: Devrait retourner -1\n" RESET);
        print_config_details(&config);
    }
    printf("\n");
}

void test_well_known_sites(void) {
    printf(BLUE "Test 5: Sites web connus\n" RESET);
    
    const char *sites[] = {
        "github.com",
        "cloudflare.com",
        "example.com"
    };
    
    int pass_count = 0;
    int total = sizeof(sites) / sizeof(sites[0]);
    
    for (int i = 0; i < total; i++) {
        t_ping_config config = create_clean_config();
        config.hostname = (char *)sites[i];
        
        int result = resolve_hostname(&config);
        
        if (result == 0 && is_valid_ipv4(config.resolved_ip)) {
            printf(GREEN "  ✓ %s -> %s\n" RESET, sites[i], config.resolved_ip);
            pass_count++;
        } else {
            printf(RED "  ✗ %s: échec\n" RESET, sites[i]);
        }
    }
    
    printf("\n");
    if (pass_count == total) {
        printf(GREEN "✓ PASS: Tous les sites résolus (%d/%d)\n" RESET, pass_count, total);
    } else {
        printf(YELLOW "⚠ PARTIAL: %d/%d sites résolus\n" RESET, pass_count, total);
    }
    printf("\n");
}

void test_loopback_variations(void) {
    printf(BLUE "Test 6: Variations de loopback\n" RESET);
    
    const char *loopbacks[] = {
        "127.0.0.1",
        "localhost"
    };
    
    int pass_count = 0;
    int total = sizeof(loopbacks) / sizeof(loopbacks[0]);
    
    for (int i = 0; i < total; i++) {
        t_ping_config config = create_clean_config();
        config.hostname = (char *)loopbacks[i];
        
        int result = resolve_hostname(&config);
        
        if (result == 0) {
            // Vérifier que l'IP commence par 127.
            if (strncmp(config.resolved_ip, "127.", 4) == 0) {
                printf(GREEN "  ✓ %s -> %s\n" RESET, loopbacks[i], config.resolved_ip);
                pass_count++;
            } else {
                printf(RED "  ✗ %s -> %s (devrait être 127.x.x.x)\n" RESET, 
                       loopbacks[i], config.resolved_ip);
            }
        } else {
            printf(RED "  ✗ %s: échec résolution\n" RESET, loopbacks[i]);
        }
    }
    
    printf("\n");
    if (pass_count == total) {
        printf(GREEN "✓ PASS: Toutes les variations loopback OK\n" RESET);
    } else {
        printf(RED "✗ FAIL: Certaines variations ont échoué\n" RESET);
    }
    printf("\n");
}

void test_struct_consistency(void) {
    printf(BLUE "Test 7: Cohérence des structures\n" RESET);
    
    t_ping_config config = create_clean_config();
    config.hostname = "8.8.8.8";
    
    int result = resolve_hostname(&config);
    
    if (result == 0) {
        // Vérifier que resolved_ip et dest_addr contiennent la même IP
        char ip_from_struct[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &config.dest_addr.sin_addr, ip_from_struct, INET_ADDRSTRLEN);
        
        if (strcmp(config.resolved_ip, ip_from_struct) == 0 &&
            config.dest_addr.sin_family == AF_INET) {
            printf(GREEN "✓ PASS: Cohérence entre resolved_ip et dest_addr\n" RESET);
            print_config_details(&config);
        } else {
            printf(RED "✗ FAIL: Incohérence détectée\n" RESET);
            print_config_details(&config);
        }
    } else {
        printf(RED "✗ FAIL: Résolution échouée\n" RESET);
    }
    printf("\n");
}

void test_empty_hostname(void) {
    printf(BLUE "Test 8: Hostname vide\n" RESET);
    
    t_ping_config config = create_clean_config();
    config.hostname = "";
    
    int result = resolve_hostname(&config);
    
    if (result == -1) {
        printf(GREEN "✓ PASS: Erreur détectée pour hostname vide\n" RESET);
    } else {
        printf(RED "✗ FAIL: Devrait retourner -1\n" RESET);
    }
    printf("\n");
}

void test_null_hostname(void) {
    printf(BLUE "Test 9: Hostname NULL\n" RESET);
    printf(YELLOW "⚠ Attention: Ce test peut causer un segfault si non géré\n" RESET);
    
    t_ping_config config = create_clean_config();
    config.hostname = NULL;
    
    // Note: Ce test peut segfault si la fonction ne vérifie pas NULL
    int result = resolve_hostname(&config);
    
    if (result == -1) {
        printf(GREEN "✓ PASS: Erreur gérée pour hostname NULL\n" RESET);
    } else {
        printf(RED "✗ FAIL: Devrait retourner -1 ou segfault\n" RESET);
    }
    printf("\n");
}

void test_ipv4_edge_cases(void) {
    printf(BLUE "Test 10: Cas limites IPv4\n" RESET);
    
    const char *ips[] = {
        "0.0.0.0",
        "255.255.255.255",
        "192.168.1.1"
    };
    
    int pass_count = 0;
    int total = sizeof(ips) / sizeof(ips[0]);
    
    for (int i = 0; i < total; i++) {
        t_ping_config config = create_clean_config();
        config.hostname = (char *)ips[i];
        
        int result = resolve_hostname(&config);
        
        if (result == 0 && strcmp(config.resolved_ip, ips[i]) == 0) {
            printf(GREEN "  ✓ %s conservée\n" RESET, ips[i]);
            pass_count++;
        } else {
            printf(RED "  ✗ %s: problème\n" RESET, ips[i]);
        }
    }
    
    printf("\n");
    if (pass_count == total) {
        printf(GREEN "✓ PASS: Tous les cas limites OK\n" RESET);
    } else {
        printf(RED "✗ FAIL: Certains cas ont échoué\n" RESET);
    }
    printf("\n");
}

int main(void) {
    printf("\n");
    printf("==========================================\n");
    printf("   TESTS POUR resolve_hostname()\n");
    printf("==========================================\n");
    printf(YELLOW "Note: Ces tests nécessitent une connexion réseau\n" RESET);
    printf("\n");

    test_localhost();
    test_direct_ip();
    test_loopback_variations();
    test_struct_consistency();
    test_ipv4_edge_cases();
    test_empty_hostname();
    
    printf(YELLOW "Tests réseau (peuvent échouer sans connexion):\n" RESET);
    test_google_dns();
    test_well_known_sites();
    
    printf(YELLOW "Tests potentiellement dangereux:\n" RESET);
    test_invalid_hostname();
    // test_null_hostname(); // Décommentez avec précaution
    
    printf("==========================================\n");
    printf("         FIN DES TESTS\n");
    printf("==========================================\n\n");
    
    printf(YELLOW "Rappel: Certains tests nécessitent:\n" RESET);
    printf("  - Une connexion réseau active\n");
    printf("  - Des permissions root pour SOCK_RAW (selon OS)\n");
    printf("  - DNS fonctionnel\n\n");

    return 0;
}