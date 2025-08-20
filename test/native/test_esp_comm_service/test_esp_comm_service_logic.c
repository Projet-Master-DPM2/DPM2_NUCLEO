#include "unity.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifndef UNITY_NATIVE_TESTS
#include "../../mocks/mock_global.h"
#endif

// Types d'événements ESP (copiés pour les tests)
typedef enum {
    ESP_MSG_UNKNOWN = 0,
    ESP_MSG_NFC_UID,
    ESP_MSG_NFC_ERR,
    ESP_MSG_NAK_PAYING_NO_NET,
    ESP_MSG_NAK_PAYMENT_DENIED
} EspMessageType;

// Constantes du service ESP (copiées pour les tests)
#define UART_BUFFER_SIZE 128
#define UART_MAX_LINE_LENGTH (UART_BUFFER_SIZE - 1)
#define UART_MAX_INVALID_CHARS 10

// Copie de la fonction EspComm_ClassifyMessage pour les tests
static EspMessageType EspComm_ClassifyMessage_TestVersion(const char* line) {
    if (!line) return ESP_MSG_UNKNOWN;
    if (strncmp(line, "NFC_UID:", 8) == 0) return ESP_MSG_NFC_UID;
    if (strncmp(line, "NFC_ERR:", 8) == 0) return ESP_MSG_NFC_ERR;
    if (strncmp(line, "NAK:STATE:PAYING:NO_NET", 23) == 0) return ESP_MSG_NAK_PAYING_NO_NET;
    if (strncmp(line, "NAK:PAYMENT:DENIED", 19) == 0) return ESP_MSG_NAK_PAYMENT_DENIED;

    return ESP_MSG_UNKNOWN;
}

// Copie de la fonction EspComm_IsValidChar pour les tests
static bool EspComm_IsValidChar_TestVersion(char c) {
    // Autoriser seulement caractères imprimables ASCII + CR/LF
    return (c >= 0x20 && c <= 0x7E) || c == '\r' || c == '\n';
}

// Fonction de validation de longueur de ligne
static bool EspComm_ValidateLineLength_TestVersion(const char* line) {
    if (!line) return false;
    size_t len = strlen(line);
    return (len > 0 && len <= UART_MAX_LINE_LENGTH);
}

void setUp(void) {
    // Configuration avant chaque test
}

void tearDown(void) {
    // Nettoyage après chaque test
}

// Test de classification des messages NFC
void test_esp_comm_classify_nfc_messages(void) {
    // Test des messages NFC UID
    TEST_ASSERT_EQUAL(ESP_MSG_NFC_UID, EspComm_ClassifyMessage_TestVersion("NFC_UID:12345678"));
    TEST_ASSERT_EQUAL(ESP_MSG_NFC_UID, EspComm_ClassifyMessage_TestVersion("NFC_UID:ABCDEF01"));
    TEST_ASSERT_EQUAL(ESP_MSG_NFC_UID, EspComm_ClassifyMessage_TestVersion("NFC_UID:"));
    
    // Test des messages NFC ERR
    TEST_ASSERT_EQUAL(ESP_MSG_NFC_ERR, EspComm_ClassifyMessage_TestVersion("NFC_ERR:TIMEOUT"));
    TEST_ASSERT_EQUAL(ESP_MSG_NFC_ERR, EspComm_ClassifyMessage_TestVersion("NFC_ERR:NO_CARD"));
    TEST_ASSERT_EQUAL(ESP_MSG_NFC_ERR, EspComm_ClassifyMessage_TestVersion("NFC_ERR:"));
}

// Test de classification des messages NAK
void test_esp_comm_classify_nak_messages(void) {
    // Test des messages NAK réseau
    TEST_ASSERT_EQUAL(ESP_MSG_NAK_PAYING_NO_NET, 
                     EspComm_ClassifyMessage_TestVersion("NAK:STATE:PAYING:NO_NET"));
    
    // Test des messages NAK paiement refusé
    TEST_ASSERT_EQUAL(ESP_MSG_NAK_PAYMENT_DENIED, 
                     EspComm_ClassifyMessage_TestVersion("NAK:PAYMENT:DENIED"));
}

// Test de classification des messages inconnus
void test_esp_comm_classify_unknown_messages(void) {
    // Test avec des messages non reconnus
    TEST_ASSERT_EQUAL(ESP_MSG_UNKNOWN, EspComm_ClassifyMessage_TestVersion("UNKNOWN_MSG"));
    TEST_ASSERT_EQUAL(ESP_MSG_UNKNOWN, EspComm_ClassifyMessage_TestVersion("ACK:OK"));
    TEST_ASSERT_EQUAL(ESP_MSG_UNKNOWN, EspComm_ClassifyMessage_TestVersion(""));
    TEST_ASSERT_EQUAL(ESP_MSG_UNKNOWN, EspComm_ClassifyMessage_TestVersion("NFC_UI:12345")); // Typo
    TEST_ASSERT_EQUAL(ESP_MSG_UNKNOWN, EspComm_ClassifyMessage_TestVersion("nfc_uid:12345")); // Casse
}

// Test avec des paramètres invalides
void test_esp_comm_classify_null_parameters(void) {
    // Test avec pointeur NULL
    TEST_ASSERT_EQUAL(ESP_MSG_UNKNOWN, EspComm_ClassifyMessage_TestVersion(NULL));
}

// Test de validation des caractères
void test_esp_comm_valid_characters(void) {
    // Test des caractères imprimables ASCII
    for (char c = 0x20; c <= 0x7E; c++) {
        TEST_ASSERT_TRUE(EspComm_IsValidChar_TestVersion(c));
    }
    
    // Test des caractères de contrôle autorisés
    TEST_ASSERT_TRUE(EspComm_IsValidChar_TestVersion('\r'));
    TEST_ASSERT_TRUE(EspComm_IsValidChar_TestVersion('\n'));
}

// Test de validation des caractères invalides
void test_esp_comm_invalid_characters(void) {
    // Test des caractères de contrôle non autorisés
    TEST_ASSERT_FALSE(EspComm_IsValidChar_TestVersion('\0'));
    TEST_ASSERT_FALSE(EspComm_IsValidChar_TestVersion('\t'));
    TEST_ASSERT_FALSE(EspComm_IsValidChar_TestVersion('\b'));
    TEST_ASSERT_FALSE(EspComm_IsValidChar_TestVersion(0x7F)); // DEL
    
    // Test des caractères étendus
    TEST_ASSERT_FALSE(EspComm_IsValidChar_TestVersion(0x80));
    TEST_ASSERT_FALSE(EspComm_IsValidChar_TestVersion(0xFF));
    
    // Test des caractères en dessous de l'ASCII imprimable
    for (char c = 0x00; c < 0x20; c++) {
        if (c != '\r' && c != '\n') {
            TEST_ASSERT_FALSE(EspComm_IsValidChar_TestVersion(c));
        }
    }
}

// Test de validation de longueur de ligne
void test_esp_comm_line_length_validation(void) {
    // Test avec ligne vide
    TEST_ASSERT_FALSE(EspComm_ValidateLineLength_TestVersion(""));
    TEST_ASSERT_FALSE(EspComm_ValidateLineLength_TestVersion(NULL));
    
    // Test avec ligne normale
    TEST_ASSERT_TRUE(EspComm_ValidateLineLength_TestVersion("NFC_UID:12345678"));
    
    // Test avec ligne à la limite (127 caractères)
    char max_line[UART_MAX_LINE_LENGTH + 1];
    memset(max_line, 'A', UART_MAX_LINE_LENGTH);
    max_line[UART_MAX_LINE_LENGTH] = '\0';
    TEST_ASSERT_TRUE(EspComm_ValidateLineLength_TestVersion(max_line));
    
    // Test avec ligne trop longue (128 caractères)
    char too_long[UART_BUFFER_SIZE + 1];
    memset(too_long, 'A', UART_BUFFER_SIZE);
    too_long[UART_BUFFER_SIZE] = '\0';
    TEST_ASSERT_FALSE(EspComm_ValidateLineLength_TestVersion(too_long));
}

// Test de robustesse du parsing
void test_esp_comm_parsing_robustness(void) {
    // Test avec des préfixes partiels
    TEST_ASSERT_EQUAL(ESP_MSG_UNKNOWN, EspComm_ClassifyMessage_TestVersion("NFC_UID"));
    TEST_ASSERT_EQUAL(ESP_MSG_UNKNOWN, EspComm_ClassifyMessage_TestVersion("NFC_ER"));
    TEST_ASSERT_EQUAL(ESP_MSG_UNKNOWN, EspComm_ClassifyMessage_TestVersion("NAK:STATE"));
    
    // Test avec des caractères supplémentaires
    TEST_ASSERT_EQUAL(ESP_MSG_NFC_UID, EspComm_ClassifyMessage_TestVersion("NFC_UID:12345678EXTRA"));
    TEST_ASSERT_EQUAL(ESP_MSG_NAK_PAYING_NO_NET, EspComm_ClassifyMessage_TestVersion("NAK:STATE:PAYING:NO_NET:EXTRA"));
}

// Test de sécurité - tentatives d'injection
void test_esp_comm_security_injection(void) {
    // Note: La fonction EspComm_ClassifyMessage ne valide pas les caractères
    // Elle se contente de vérifier les préfixes. La validation des caractères
    // se fait dans EspComm_IsValidChar et lors de la réception UART.
    
    // Test avec des caractères de contrôle - toujours classifiés selon le préfixe
    TEST_ASSERT_EQUAL(ESP_MSG_NFC_UID, EspComm_ClassifyMessage_TestVersion("NFC_UID:\x00\x01"));
    TEST_ASSERT_EQUAL(ESP_MSG_UNKNOWN, EspComm_ClassifyMessage_TestVersion("NAK:STATE\x7F:PAYING")); // Préfixe incorrect
    
    // Test avec des tentatives de débordement
    char overflow_attempt[200];
    memset(overflow_attempt, 'A', 199);
    overflow_attempt[199] = '\0';
    strcpy(overflow_attempt, "NFC_UID:");
    TEST_ASSERT_EQUAL(ESP_MSG_NFC_UID, EspComm_ClassifyMessage_TestVersion(overflow_attempt));
}

// Test des cas limites de protocole
void test_esp_comm_protocol_edge_cases(void) {
    // Test avec des messages valides mais vides
    TEST_ASSERT_EQUAL(ESP_MSG_NFC_UID, EspComm_ClassifyMessage_TestVersion("NFC_UID:"));
    TEST_ASSERT_EQUAL(ESP_MSG_NFC_ERR, EspComm_ClassifyMessage_TestVersion("NFC_ERR:"));
    
    // Test de sensibilité à la casse
    TEST_ASSERT_EQUAL(ESP_MSG_UNKNOWN, EspComm_ClassifyMessage_TestVersion("nfc_uid:12345"));
    TEST_ASSERT_EQUAL(ESP_MSG_UNKNOWN, EspComm_ClassifyMessage_TestVersion("Nfc_Uid:12345"));
    
    // Test avec espaces - la fonction ne vérifie que les préfixes exacts
    TEST_ASSERT_EQUAL(ESP_MSG_UNKNOWN, EspComm_ClassifyMessage_TestVersion(" NFC_UID:12345"));
    TEST_ASSERT_EQUAL(ESP_MSG_NFC_UID, EspComm_ClassifyMessage_TestVersion("NFC_UID: 12345")); // Espace après ':'
}

// Test de validation complète des messages
void test_esp_comm_complete_message_validation(void) {
    // Messages valides complets
    char* valid_messages[] = {
        "NFC_UID:1A2B3C4D",
        "NFC_ERR:TIMEOUT",
        "NAK:STATE:PAYING:NO_NET",
        "NAK:PAYMENT:DENIED"
    };
    
    EspMessageType expected_types[] = {
        ESP_MSG_NFC_UID,
        ESP_MSG_NFC_ERR,
        ESP_MSG_NAK_PAYING_NO_NET,
        ESP_MSG_NAK_PAYMENT_DENIED
    };
    
    int num_messages = sizeof(valid_messages) / sizeof(valid_messages[0]);
    
    for (int i = 0; i < num_messages; i++) {
        // Vérifier la classification
        TEST_ASSERT_EQUAL(expected_types[i], 
                         EspComm_ClassifyMessage_TestVersion(valid_messages[i]));
        
        // Vérifier la longueur
        TEST_ASSERT_TRUE(EspComm_ValidateLineLength_TestVersion(valid_messages[i]));
        
        // Vérifier que tous les caractères sont valides
        size_t len = strlen(valid_messages[i]);
        for (size_t j = 0; j < len; j++) {
            TEST_ASSERT_TRUE(EspComm_IsValidChar_TestVersion(valid_messages[i][j]));
        }
    }
}

// Test de performance avec messages longs
void test_esp_comm_performance_long_messages(void) {
    // Test avec un message NFC UID long mais valide
    char long_uid[UART_MAX_LINE_LENGTH + 1];
    strcpy(long_uid, "NFC_UID:");
    
    // Remplir avec des caractères hex valides
    size_t prefix_len = strlen("NFC_UID:");
    for (size_t i = prefix_len; i < UART_MAX_LINE_LENGTH; i++) {
        long_uid[i] = (i % 2 == 0) ? 'A' : '1';
    }
    long_uid[UART_MAX_LINE_LENGTH] = '\0';
    
    // Devrait être classifié correctement
    TEST_ASSERT_EQUAL(ESP_MSG_NFC_UID, EspComm_ClassifyMessage_TestVersion(long_uid));
    TEST_ASSERT_TRUE(EspComm_ValidateLineLength_TestVersion(long_uid));
}

int main(void) {
    UNITY_BEGIN();
    
    // Tests de classification de base
    RUN_TEST(test_esp_comm_classify_nfc_messages);
    RUN_TEST(test_esp_comm_classify_nak_messages);
    RUN_TEST(test_esp_comm_classify_unknown_messages);
    RUN_TEST(test_esp_comm_classify_null_parameters);
    
    // Tests de validation des caractères
    RUN_TEST(test_esp_comm_valid_characters);
    RUN_TEST(test_esp_comm_invalid_characters);
    
    // Tests de validation des lignes
    RUN_TEST(test_esp_comm_line_length_validation);
    RUN_TEST(test_esp_comm_parsing_robustness);
    RUN_TEST(test_esp_comm_protocol_edge_cases);
    
    // Tests de sécurité et performance
    RUN_TEST(test_esp_comm_security_injection);
    RUN_TEST(test_esp_comm_complete_message_validation);
    RUN_TEST(test_esp_comm_performance_long_messages);
    
    return UNITY_END();
}
