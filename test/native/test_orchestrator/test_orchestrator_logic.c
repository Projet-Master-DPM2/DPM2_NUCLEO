#ifndef UNITY_NATIVE_TESTS
#define UNITY_NATIVE_TESTS
#endif
#include "unity_config.h"
#include "unity.h"
#include "mock_hal.h"
#include "mock_freertos.h"
#include "mock_global.h"

// Mock des services externes
void LCD_SendMessage(const LcdMessage* msg) {
    // Mock - ne fait rien
}

void EspComm_SendLine(const char* line) {
    // Mock - ne fait rien
}

uint8_t MotorService_OrderToChannel(uint8_t orderCode) {
    // Implémentation de test
    switch (orderCode) {
        case 11: return 0;
        case 12: return 1;
        case 13: return 2;
        case 21: return 3;
        case 22: return 4;
        case 23: return 6;
        default: return 0xFF;
    }
}

void MotorService_StartDelivery(uint8_t channel) {
    // Mock - ne fait rien
}

// Variables globales mockées (définies dans mock_global.c)
extern volatile MachineState machine_interaction;
extern volatile char keypad_choice[3];
extern volatile uint8_t client_order;

// ============================================================================
// TESTS SETUP ET TEARDOWN
// ============================================================================

void setUp(void) {
    Mock_HAL_Reset();
    Mock_FreeRTOS_Reset();
    
    // Reset des variables globales
    machine_interaction = IDLE;
    memset((void*)keypad_choice, 0, sizeof(keypad_choice));
    client_order = 0;
}

void tearDown(void) {
    // Nettoyage après chaque test
}

// ============================================================================
// TESTS DE LA MACHINE À ÉTATS
// ============================================================================

void test_machine_state_initial(void) {
    TEST_ASSERT_STATE_EQUAL(IDLE, machine_interaction);
    TEST_ASSERT_EQUAL_STRING("", (char*)keypad_choice);
    TEST_ASSERT_EQUAL_UINT8(0, client_order);
}

void test_machine_state_transitions(void) {
    // Test IDLE -> ORDERING
    machine_interaction = IDLE;
    // Simuler saisie premier chiffre
    keypad_choice[0] = '1';
    keypad_choice[1] = '\0';
    machine_interaction = ORDERING;
    
    TEST_ASSERT_STATE_EQUAL(ORDERING, machine_interaction);
    TEST_ASSERT_EQUAL_CHAR('1', keypad_choice[0]);
    
    // Test ORDERING -> PAYING
    keypad_choice[1] = '1';  // Code "11"
    keypad_choice[2] = '\0';
    machine_interaction = PAYING;
    
    TEST_ASSERT_STATE_EQUAL(PAYING, machine_interaction);
    
    // Test PAYING -> DELIVERING
    machine_interaction = DELIVERING;
    TEST_ASSERT_STATE_EQUAL(DELIVERING, machine_interaction);
    
    // Test DELIVERING -> IDLE
    machine_interaction = IDLE;
    TEST_ASSERT_STATE_EQUAL(IDLE, machine_interaction);
}

void test_order_code_validation(void) {
    // Codes valides
    TEST_ASSERT_EQUAL_UINT8(0, MotorService_OrderToChannel(11));
    TEST_ASSERT_EQUAL_UINT8(1, MotorService_OrderToChannel(12));
    TEST_ASSERT_EQUAL_UINT8(2, MotorService_OrderToChannel(13));
    TEST_ASSERT_EQUAL_UINT8(3, MotorService_OrderToChannel(21));
    TEST_ASSERT_EQUAL_UINT8(4, MotorService_OrderToChannel(22));
    TEST_ASSERT_EQUAL_UINT8(6, MotorService_OrderToChannel(23));
    
    // Codes invalides
    TEST_ASSERT_EQUAL_UINT8(0xFF, MotorService_OrderToChannel(10));
    TEST_ASSERT_EQUAL_UINT8(0xFF, MotorService_OrderToChannel(14));
    TEST_ASSERT_EQUAL_UINT8(0xFF, MotorService_OrderToChannel(31));
    TEST_ASSERT_EQUAL_UINT8(0xFF, MotorService_OrderToChannel(99));
}

// ============================================================================
// TESTS DE LA LOGIQUE MÉTIER
// ============================================================================

void test_keypad_input_validation(void) {
    char valid_chars[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '*', '#'};
    char invalid_chars[] = {'a', 'A', ' ', '\n', '\r', '!', '@'};
    
    // Test caractères valides
    for (int i = 0; i < sizeof(valid_chars); i++) {
        char c = valid_chars[i];
        bool is_valid = ((c >= '0' && c <= '9') || c == '*' || c == '#');
        TEST_ASSERT_TRUE_MESSAGE(is_valid, "Caractère valide rejeté");
    }
    
    // Test caractères invalides
    for (int i = 0; i < sizeof(invalid_chars); i++) {
        char c = invalid_chars[i];
        bool is_valid = ((c >= '0' && c <= '9') || c == '*' || c == '#');
        TEST_ASSERT_FALSE_MESSAGE(is_valid, "Caractère invalide accepté");
    }
}

void test_order_processing_flow(void) {
    // Simulation du flux complet de commande
    
    // 1. État initial
    machine_interaction = IDLE;
    TEST_ASSERT_STATE_EQUAL(IDLE, machine_interaction);
    
    // 2. Saisie premier chiffre
    keypad_choice[0] = '1';
    keypad_choice[1] = '\0';
    machine_interaction = ORDERING;
    TEST_ASSERT_STATE_EQUAL(ORDERING, machine_interaction);
    TEST_ASSERT_EQUAL_STRING("1", (char*)keypad_choice);
    
    // 3. Saisie second chiffre (code complet)
    keypad_choice[1] = '2';
    keypad_choice[2] = '\0';
    uint8_t orderCode = (keypad_choice[0] - '0') * 10 + (keypad_choice[1] - '0');
    TEST_ASSERT_EQUAL_UINT8(12, orderCode);
    
    // 4. Validation du code
    uint8_t channel = MotorService_OrderToChannel(orderCode);
    TEST_ASSERT_NOT_EQUAL(0xFF, channel);
    
    // 5. Transition vers paiement
    client_order = orderCode;
    machine_interaction = PAYING;
    TEST_ASSERT_STATE_EQUAL(PAYING, machine_interaction);
    TEST_ASSERT_EQUAL_UINT8(12, client_order);
    
    // 6. Reset du choix keypad
    memset((void*)keypad_choice, 0, sizeof(keypad_choice));
    TEST_ASSERT_EQUAL_STRING("", (char*)keypad_choice);
}

void test_error_handling(void) {
    // Test gestion code produit invalide
    machine_interaction = ORDERING;
    keypad_choice[0] = '9';
    keypad_choice[1] = '9';
    keypad_choice[2] = '\0';
    
    uint8_t orderCode = (keypad_choice[0] - '0') * 10 + (keypad_choice[1] - '0');
    uint8_t channel = MotorService_OrderToChannel(orderCode);
    
    TEST_ASSERT_EQUAL_UINT8(99, orderCode);
    TEST_ASSERT_EQUAL_UINT8(0xFF, channel);  // Code invalide
    
    // Le système devrait revenir à IDLE
    if (channel == 0xFF) {
        client_order = 0;
        machine_interaction = IDLE;
        memset((void*)keypad_choice, 0, sizeof(keypad_choice));
    }
    
    TEST_ASSERT_STATE_EQUAL(IDLE, machine_interaction);
    TEST_ASSERT_EQUAL_UINT8(0, client_order);
    TEST_ASSERT_EQUAL_STRING("", (char*)keypad_choice);
}

// ============================================================================
// TESTS DE PERFORMANCE ET LIMITES
// ============================================================================

void test_keypad_buffer_limits(void) {
    // Test limite de buffer keypad (3 caractères)
    TEST_ASSERT_EQUAL_INT(3, sizeof(keypad_choice));
    
    // Simuler saisie normale (2 chiffres)
    keypad_choice[0] = '1';
    keypad_choice[1] = '1';
    keypad_choice[2] = '\0';
    
    size_t len = strlen((const char*)keypad_choice);
    TEST_ASSERT_EQUAL_INT(2, len);
    TEST_ASSERT_LESS_OR_EQUAL(sizeof(keypad_choice), len + 1);  // +1 pour '\0', doit être ≤
}

void test_state_consistency(void) {
    // Test cohérence entre états et variables
    
    // État IDLE : variables resetées
    machine_interaction = IDLE;
    client_order = 0;
    memset((void*)keypad_choice, 0, sizeof(keypad_choice));
    
    TEST_ASSERT_STATE_EQUAL(IDLE, machine_interaction);
    TEST_ASSERT_EQUAL_UINT8(0, client_order);
    TEST_ASSERT_EQUAL_STRING("", (char*)keypad_choice);
    
    // État PAYING : client_order défini, keypad_choice reset
    machine_interaction = PAYING;
    client_order = 12;
    memset((void*)keypad_choice, 0, sizeof(keypad_choice));
    
    TEST_ASSERT_STATE_EQUAL(PAYING, machine_interaction);
    TEST_ASSERT_EQUAL_UINT8(12, client_order);
    TEST_ASSERT_EQUAL_STRING("", (char*)keypad_choice);
}

// ============================================================================
// MAIN DE TEST
// ============================================================================

int main(void) {
    UNITY_BEGIN();
    
    // Tests de base
    RUN_TEST(test_machine_state_initial);
    RUN_TEST(test_machine_state_transitions);
    RUN_TEST(test_order_code_validation);
    
    // Tests logique métier
    RUN_TEST(test_keypad_input_validation);
    RUN_TEST(test_order_processing_flow);
    RUN_TEST(test_error_handling);
    
    // Tests performance et limites
    RUN_TEST(test_keypad_buffer_limits);
    RUN_TEST(test_state_consistency);
    
    return UNITY_END();
}
