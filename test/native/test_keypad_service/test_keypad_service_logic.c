#include "unity.h"
#include <stdint.h>
#include <stdbool.h>

// Inclure les mocks pour HAL_GetTick
#include "../../mocks/mock_hal.h"

#ifndef UNITY_NATIVE_TESTS
#include "../../mocks/mock_global.h"
#endif

// Constantes du keypad service (copiées pour les tests)
#define KEYPAD_MAX_INVALID_ATTEMPTS 10
#define KEYPAD_LOCKOUT_TIME_MS 5000
#define KEYPAD_MIN_PRESS_INTERVAL_MS 100

// Variables statiques simulées pour les tests
static uint32_t mock_invalidAttempts = 0;
static uint32_t mock_lastKeyPressTime = 0;
static uint32_t mock_lockoutStartTime = 0;

// Copie de la fonction Keypad_ValidateInput pour les tests
// (évite les dépendances STM32)
static bool Keypad_ValidateInput_TestVersion(char key) {
    uint32_t currentTime = HAL_GetTick();
    
    // Vérifier si en période de lockout
    if (mock_lockoutStartTime != 0) {
        if ((currentTime - mock_lockoutStartTime) < KEYPAD_LOCKOUT_TIME_MS) {
            return false; // Encore en lockout
        } else {
            // Fin du lockout, reset des compteurs
            mock_lockoutStartTime = 0;
            mock_invalidAttempts = 0;
        }
    }
    
    // Rate limiting : éviter les presses trop rapides
    if (mock_lastKeyPressTime != 0 && (currentTime - mock_lastKeyPressTime) < KEYPAD_MIN_PRESS_INTERVAL_MS) {
        return false;
    }
    
    // Validation caractère
    if (!((key >= '0' && key <= '9') || key == '*' || key == '#')) {
        mock_invalidAttempts++;
        
        if (mock_invalidAttempts >= KEYPAD_MAX_INVALID_ATTEMPTS) {
            mock_lockoutStartTime = currentTime;
        }
        return false;
    }
    
    // Caractère valide
    mock_lastKeyPressTime = currentTime;
    if (mock_invalidAttempts > 0) {
        mock_invalidAttempts = 0; // Reset sur succès
    }
    return true;
}

// Fonction de reset pour les tests
static void Mock_Keypad_Reset(void) {
    mock_invalidAttempts = 0;
    mock_lastKeyPressTime = 0;
    mock_lockoutStartTime = 0;
    Mock_HAL_SetTick(0);
}

void setUp(void) {
    // Réinitialiser l'état avant chaque test
    Mock_Keypad_Reset();
}

void tearDown(void) {
    // Nettoyage après chaque test
}

// Test de validation des caractères valides
void test_keypad_valid_characters(void) {
    Mock_HAL_SetTick(1000);
    
    // Test des chiffres 0-9
    for (char c = '0'; c <= '9'; c++) {
        Mock_HAL_SetTick(HAL_GetTick() + 200); // Éviter le rate limiting
        TEST_ASSERT_TRUE(Keypad_ValidateInput_TestVersion(c));
    }
    
    // Test des caractères spéciaux
    Mock_HAL_SetTick(HAL_GetTick() + 200);
    TEST_ASSERT_TRUE(Keypad_ValidateInput_TestVersion('*'));
    Mock_HAL_SetTick(HAL_GetTick() + 200);
    TEST_ASSERT_TRUE(Keypad_ValidateInput_TestVersion('#'));
}

// Test de validation des caractères invalides
void test_keypad_invalid_characters(void) {
    Mock_HAL_SetTick(1000);
    
    // Test de caractères invalides
    char invalid_chars[] = {'a', 'A', '!', '@', ' '};
    int num_invalid = sizeof(invalid_chars) / sizeof(invalid_chars[0]);
    
    for (int i = 0; i < num_invalid; i++) {
        Mock_HAL_SetTick(HAL_GetTick() + 200); // Éviter le rate limiting
        TEST_ASSERT_FALSE(Keypad_ValidateInput_TestVersion(invalid_chars[i]));
    }
    
    // Vérifier que les tentatives invalides sont comptées
    TEST_ASSERT_EQUAL_UINT32(num_invalid, mock_invalidAttempts);
}

// Test du rate limiting
void test_keypad_rate_limiting(void) {
    Mock_HAL_SetTick(1000);
    
    // Première pression valide
    TEST_ASSERT_TRUE(Keypad_ValidateInput_TestVersion('1'));
    
    // Pression trop rapide (< 100ms)
    Mock_HAL_SetTick(HAL_GetTick() + 50);
    TEST_ASSERT_FALSE(Keypad_ValidateInput_TestVersion('2'));
    
    // Pression après délai suffisant
    Mock_HAL_SetTick(HAL_GetTick() + 100); // Total 150ms depuis la première
    TEST_ASSERT_TRUE(Keypad_ValidateInput_TestVersion('2'));
}

// Test du mécanisme de lockout
void test_keypad_lockout_mechanism(void) {
    Mock_HAL_SetTick(1000);
    
    // Générer 9 tentatives invalides (pas encore de lockout)
    for (int i = 0; i < 9; i++) {
        Mock_HAL_SetTick(HAL_GetTick() + 200);
        TEST_ASSERT_FALSE(Keypad_ValidateInput_TestVersion('a'));
    }
    
    // Vérifier qu'on n'est pas encore en lockout
    TEST_ASSERT_EQUAL_UINT32(0, mock_lockoutStartTime);
    
    // 10ème tentative invalide → déclenchement du lockout
    Mock_HAL_SetTick(HAL_GetTick() + 200);
    TEST_ASSERT_FALSE(Keypad_ValidateInput_TestVersion('b'));
    TEST_ASSERT_NOT_EQUAL(0, mock_lockoutStartTime);
    
    // Vérifier que même les caractères valides sont rejetés pendant le lockout
    Mock_HAL_SetTick(HAL_GetTick() + 1000); // 1 seconde plus tard
    TEST_ASSERT_FALSE(Keypad_ValidateInput_TestVersion('1'));
    
    // Après la fin du lockout (5 secondes)
    Mock_HAL_SetTick(HAL_GetTick() + 4500); // Total 5.5 secondes depuis le lockout
    TEST_ASSERT_TRUE(Keypad_ValidateInput_TestVersion('1'));
    
    // Vérifier que les compteurs sont reset
    TEST_ASSERT_EQUAL_UINT32(0, mock_invalidAttempts);
    TEST_ASSERT_EQUAL_UINT32(0, mock_lockoutStartTime);
}

// Test de reset des compteurs sur succès
void test_keypad_reset_on_success(void) {
    Mock_HAL_SetTick(1000);
    
    // Générer quelques tentatives invalides
    for (int i = 0; i < 5; i++) {
        Mock_HAL_SetTick(HAL_GetTick() + 200);
        TEST_ASSERT_FALSE(Keypad_ValidateInput_TestVersion('x'));
    }
    
    TEST_ASSERT_EQUAL_UINT32(5, mock_invalidAttempts);
    
    // Une tentative valide devrait reset le compteur
    Mock_HAL_SetTick(HAL_GetTick() + 200);
    TEST_ASSERT_TRUE(Keypad_ValidateInput_TestVersion('1'));
    TEST_ASSERT_EQUAL_UINT32(0, mock_invalidAttempts);
}

// Test des cas limites du rate limiting
void test_keypad_rate_limiting_edge_cases(void) {
    Mock_HAL_SetTick(1000);
    
    // Première pression
    TEST_ASSERT_TRUE(Keypad_ValidateInput_TestVersion('1'));
    
    // Exactement à la limite (100ms)
    Mock_HAL_SetTick(HAL_GetTick() + 100);
    TEST_ASSERT_TRUE(Keypad_ValidateInput_TestVersion('2'));
    
    // Juste en dessous de la limite (99ms)
    Mock_HAL_SetTick(HAL_GetTick() + 99);
    TEST_ASSERT_FALSE(Keypad_ValidateInput_TestVersion('3'));
    
    // Juste au dessus de la limite (101ms depuis la dernière valide)
    Mock_HAL_SetTick(HAL_GetTick() + 2); // Total 101ms
    TEST_ASSERT_TRUE(Keypad_ValidateInput_TestVersion('3'));
}

// Test de la logique de keymap
void test_keypad_keymap_logic(void) {
    // Test de la structure du keymap (logique pure)
    char expected_keymap[4][3] = {
        {'1', '2', '3'},
        {'4', '5', '6'},
        {'7', '8', '9'},
        {'*', '0', '#'}
    };
    
    Mock_HAL_SetTick(1000);
    
    // Vérifier que tous les caractères du keymap sont valides
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 3; col++) {
            char key = expected_keymap[row][col];
            Mock_HAL_SetTick(HAL_GetTick() + 200); // Éviter rate limiting
            TEST_ASSERT_TRUE(Keypad_ValidateInput_TestVersion(key));
        }
    }
}

// Test de sécurité - tentatives d'injection
void test_keypad_security_injection(void) {
    Mock_HAL_SetTick(1000);
    
    // Test avec des caractères de contrôle
    char control_chars[] = {'\0', '\n', '\r', '\t', 0x7F, 0xFF};
    int num_control = sizeof(control_chars) / sizeof(control_chars[0]);
    
    for (int i = 0; i < num_control; i++) {
        Mock_HAL_SetTick(HAL_GetTick() + 200);
        TEST_ASSERT_FALSE(Keypad_ValidateInput_TestVersion(control_chars[i]));
    }
    
    // Vérifier que les tentatives d'injection sont comptées comme invalides
    TEST_ASSERT_EQUAL_UINT32(num_control, mock_invalidAttempts);
}

int main(void) {
    UNITY_BEGIN();
    
    // Tests de validation de base
    RUN_TEST(test_keypad_valid_characters);
    RUN_TEST(test_keypad_invalid_characters);
    
    // Tests de sécurité et rate limiting
    RUN_TEST(test_keypad_rate_limiting);
    RUN_TEST(test_keypad_rate_limiting_edge_cases);
    RUN_TEST(test_keypad_lockout_mechanism);
    RUN_TEST(test_keypad_reset_on_success);
    
    // Tests de sécurité
    RUN_TEST(test_keypad_security_injection);
    
    // Tests de logique métier
    RUN_TEST(test_keypad_keymap_logic);
    
    return UNITY_END();
}
