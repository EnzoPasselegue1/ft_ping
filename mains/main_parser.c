#include "ft_ping.h"
#include <string.h>
#include <assert.h>

// Couleurs pour l'affichage
#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define BLUE "\033[0;34m"
#define RESET "\033[0m"

// Helper pour réinitialiser optind entre les tests
void reset_getopt(void) {
    optind = 1;
    opterr = 0;
}

// Helper pour créer une config vierge
t_ping_config create_clean_config(void) {
    t_ping_config config;
    memset(&config, 0, sizeof(t_ping_config));
    return config;
}

void test_no_arguments(void) {
    printf(BLUE "Test 1: Sans arguments\n" RESET);
    
    char *argv[] = {"ft_ping"};
    int argc = 1;
    t_ping_config config = create_clean_config();
    
    int result = parse_args(argc, argv, &config);
    
    if (result == -1) {
        printf(GREEN "✓ PASS: Retourne -1 comme attendu\n" RESET);
    } else {
        printf(RED "✗ FAIL: Devrait retourner -1\n" RESET);
    }
    reset_getopt();
    printf("\n");
}

void test_host_only(void) {
    printf(BLUE "Test 2: Hostname uniquement\n" RESET);
    
    char *argv[] = {"ft_ping", "google.com"};
    int argc = 2;
    t_ping_config config = create_clean_config();
    
    int result = parse_args(argc, argv, &config);
    
    if (result == 0 && 
        config.hostname != NULL && 
        strcmp(config.hostname, "google.com") == 0 &&
        config.verbose == 0) {
        printf(GREEN "✓ PASS: Hostname correctement parsé\n" RESET);
        printf("  Hostname: %s\n", config.hostname);
        printf("  Verbose: %d\n", config.verbose);
    } else {
        printf(RED "✗ FAIL: Parsing incorrect\n" RESET);
    }
    reset_getopt();
    printf("\n");
}

void test_verbose_flag(void) {
    printf(BLUE "Test 3: Flag verbose (-v)\n" RESET);
    
    char *argv[] = {"ft_ping", "-v", "localhost"};
    int argc = 3;
    t_ping_config config = create_clean_config();
    
    int result = parse_args(argc, argv, &config);
    
    if (result == 0 && 
        config.verbose == 1 &&
        strcmp(config.hostname, "localhost") == 0) {
        printf(GREEN "✓ PASS: Verbose activé correctement\n" RESET);
        printf("  Verbose: %d\n", config.verbose);
        printf("  Hostname: %s\n", config.hostname);
    } else {
        printf(RED "✗ FAIL: Verbose non activé\n" RESET);
    }
    reset_getopt();
    printf("\n");
}

void test_ip_address(void) {
    printf(BLUE "Test 4: Adresse IP directe\n" RESET);
    
    char *argv[] = {"ft_ping", "8.8.8.8"};
    int argc = 2;
    t_ping_config config = create_clean_config();
    
    int result = parse_args(argc, argv, &config);
    
    if (result == 0 && strcmp(config.hostname, "8.8.8.8") == 0) {
        printf(GREEN "✓ PASS: IP parsée correctement\n" RESET);
        printf("  Hostname: %s\n", config.hostname);
    } else {
        printf(RED "✗ FAIL: IP non parsée\n" RESET);
    }
    reset_getopt();
    printf("\n");
}

void test_multiple_options(void) {
    printf(BLUE "Test 5: Options multiples avec hostname\n" RESET);
    
    char *argv[] = {"ft_ping", "-v", "example.com"};
    int argc = 3;
    t_ping_config config = create_clean_config();
    
    int result = parse_args(argc, argv, &config);
    
    if (result == 0 && 
        config.verbose == 1 &&
        strcmp(config.hostname, "example.com") == 0) {
        printf(GREEN "✓ PASS: Options multiples OK\n" RESET);
    } else {
        printf(RED "✗ FAIL: Options multiples incorrectes\n" RESET);
    }
    reset_getopt();
    printf("\n");
}

void test_missing_host_with_option(void) {
    printf(BLUE "Test 6: Option sans hostname\n" RESET);
    
    char *argv[] = {"ft_ping", "-v"};
    int argc = 2;
    t_ping_config config = create_clean_config();
    
    int result = parse_args(argc, argv, &config);
    
    if (result == -1) {
        printf(GREEN "✓ PASS: Erreur détectée (pas de host)\n" RESET);
    } else {
        printf(RED "✗ FAIL: Devrait retourner une erreur\n" RESET);
    }
    reset_getopt();
    printf("\n");
}

void test_invalid_option(void) {
    printf(BLUE "Test 7: Option invalide\n" RESET);
    
    char *argv[] = {"ft_ping", "-x", "google.com"};
    int argc = 3;
    t_ping_config config = create_clean_config();
    
    // Cette fonction devrait retourner -1 pour option invalide
    int result = parse_args(argc, argv, &config);
    
    if (result == -1) {
        printf(GREEN "✓ PASS: Option invalide détectée\n" RESET);
    } else {
        printf(RED "✗ FAIL: Option invalide non détectée\n" RESET);
    }
    reset_getopt();
    printf("\n");
}

void test_hostname_with_special_chars(void) {
    printf(BLUE "Test 8: Hostname avec caractères spéciaux\n" RESET);
    
    char *argv[] = {"ft_ping", "sub-domain.example.com"};
    int argc = 2;
    t_ping_config config = create_clean_config();
    
    int result = parse_args(argc, argv, &config);
    
    if (result == 0 && strcmp(config.hostname, "sub-domain.example.com") == 0) {
        printf(GREEN "✓ PASS: Hostname avec tiret parsé\n" RESET);
        printf("  Hostname: %s\n", config.hostname);
    } else {
        printf(RED "✗ FAIL: Hostname mal parsé\n" RESET);
    }
    reset_getopt();
    printf("\n");
}

void test_help_display(void) {
    printf(BLUE "Test 9: Affichage de l'aide (-?)\n" RESET);
    printf("Note: Ce test terminera le programme avec exit(0)\n");
    printf("Décommentez pour tester manuellement\n\n");
    
    // Décommentez pour tester (attention: va quitter le programme)
    /*
    char *argv[] = {"ft_ping", "-?"};
    int argc = 2;
    t_ping_config config = create_clean_config();
    parse_args(argc, argv, &config);
    */
}

int main(void) {
    printf("\n");
    printf("====================================\n");
    printf("   TESTS POUR ft_ping parse_args\n");
    printf("====================================\n\n");

    test_no_arguments();
    test_host_only();
    test_verbose_flag();
    test_ip_address();
    test_multiple_options();
    test_missing_host_with_option();
    test_invalid_option();
    test_hostname_with_special_chars();
    test_help_display();

    printf("====================================\n");
    printf("       FIN DES TESTS\n");
    printf("====================================\n\n");

    return 0;
}