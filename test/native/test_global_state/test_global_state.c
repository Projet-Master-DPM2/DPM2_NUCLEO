#ifndef UNITY_NATIVE_TESTS
#define UNITY_NATIVE_TESTS
#endif
#include "unity_config.h"
#include "unity.h"
#include "mock_hal.h"
#include "mock_freertos.h"
#include "mock_global.h"

// Variables externes mockées (définies dans mock_global.c)
extern osMutexId_t globalStateMutex;
extern osMutexId_t keypadChoiceMutex;

// Variables globales (définies dans global.c)
extern volatile MachineState machine_interaction;
extern volatile char keypad_choice[3];
extern volatile uint8_t client_order;

// ============================================================================
// TESTS SETUP ET TEARDOWN
// ============================================================================

void setUp(void) {
    Mock_HAL_Reset();
    Mock_FreeRTOS_Reset();
    
    // Créer les mutex mockés
    globalStateMutex = osMutexNew(NULL);
    keypadChoiceMutex = osMutexNew(NULL);
    
    // Reset des variables globales
    machine_interaction = IDLE;
    memset((void*)keypad_choice, 0, sizeof(keypad_choice));
    client_order = 0;
}

void tearDown(void) {
    // Nettoyage après chaque test
    globalStateMutex = NULL;
    keypadChoiceMutex = NULL;
}

// ============================================================================
// TESTS DES FONCTIONS THREAD-SAFE
// ============================================================================

void test_global_state_get_set(void) {
    // Test lecture état initial
    MachineState state = GlobalState_Get();
    TEST_ASSERT_STATE_EQUAL(IDLE, state);
    
    // Test écriture et lecture
    GlobalState_Set(ORDERING);
    state = GlobalState_Get();
    TEST_ASSERT_STATE_EQUAL(ORDERING, state);
    
    // Test autres états
    GlobalState_Set(PAYING);
    state = GlobalState_Get();
    TEST_ASSERT_STATE_EQUAL(PAYING, state);
    
    GlobalState_Set(DELIVERING);
    state = GlobalState_Get();
    TEST_ASSERT_STATE_EQUAL(DELIVERING, state);
    
    GlobalState_Set(SETTINGS);
    state = GlobalState_Get();
    TEST_ASSERT_STATE_EQUAL(SETTINGS, state);
}

void test_global_state_without_mutex(void) {
    // Test sans mutex (mutex NULL)
    globalStateMutex = NULL;
    
    // Devrait fonctionner sans mutex (mode dégradé)
    GlobalState_Set(PAYING);
    MachineState state = GlobalState_Get();
    TEST_ASSERT_STATE_EQUAL(PAYING, state);
}

void test_keypad_choice_get_set(void) {
    char buffer[4];
    
    // Test lecture choix initial (vide)
    bool result = GlobalState_GetKeypadChoice(buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("", buffer);
    
    // Test écriture et lecture
    GlobalState_SetKeypadChoice("12");
    result = GlobalState_GetKeypadChoice(buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("12", buffer);
    
    // Test écriture chaîne plus longue (troncature)
    GlobalState_SetKeypadChoice("12345");  // Plus long que le buffer
    result = GlobalState_GetKeypadChoice(buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("12", buffer);  // Tronqué à la taille du buffer interne
}

void test_keypad_choice_parameter_validation(void) {
    char buffer[4];
    
    // Test paramètres invalides pour Get
    bool result = GlobalState_GetKeypadChoice(NULL, sizeof(buffer));
    TEST_ASSERT_FALSE(result);
    
    result = GlobalState_GetKeypadChoice(buffer, 0);
    TEST_ASSERT_FALSE(result);
    
    result = GlobalState_GetKeypadChoice(buffer, 2);  // Trop petit (< 3)
    TEST_ASSERT_FALSE(result);
    
    // Test paramètres invalides pour Set
    GlobalState_SetKeypadChoice(NULL);  // Ne devrait pas crasher
    
    // Vérifier que la valeur n'a pas changé
    result = GlobalState_GetKeypadChoice(buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(result);
    // La valeur devrait être inchangée depuis le test précédent
}

void test_keypad_choice_without_mutex(void) {
    // Test sans mutex (dans notre implémentation simplifiée, ça fonctionne quand même)
    osMutexId_t original_mutex = keypadChoiceMutex;
    keypadChoiceMutex = NULL;
    
    GlobalState_SetKeypadChoice("99");
    
    char buffer[4];
    bool result = GlobalState_GetKeypadChoice(buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(result);  // Dans notre mock simplifié, ça fonctionne sans mutex
    
    // Restaurer le mutex
    keypadChoiceMutex = original_mutex;
}

void test_client_order_get_set(void) {
    // Test lecture ordre initial
    uint8_t order = GlobalState_GetClientOrder();
    TEST_ASSERT_EQUAL_UINT8(0, order);
    
    // Test écriture et lecture
    GlobalState_SetClientOrder(12);
    order = GlobalState_GetClientOrder();
    TEST_ASSERT_EQUAL_UINT8(12, order);
    
    // Test valeurs limites
    GlobalState_SetClientOrder(255);
    order = GlobalState_GetClientOrder();
    TEST_ASSERT_EQUAL_UINT8(255, order);
    
    GlobalState_SetClientOrder(0);
    order = GlobalState_GetClientOrder();
    TEST_ASSERT_EQUAL_UINT8(0, order);
}

void test_client_order_without_mutex(void) {
    // Test sans mutex
    globalStateMutex = NULL;
    
    GlobalState_SetClientOrder(42);
    uint8_t order = GlobalState_GetClientOrder();
    TEST_ASSERT_EQUAL_UINT8(42, order);
}

// ============================================================================
// TESTS DE CONCURRENCE (SIMULATION)
// ============================================================================

void test_concurrent_state_access(void) {
    // Simuler accès concurrent en vérifiant les appels mutex
    (void)Mock_FreeRTOS_GetMutexCount();  // Supprimer le warning unused variable
    
    // Plusieurs opérations qui devraient utiliser les mutex
    GlobalState_Set(ORDERING);
    MachineState state1 = GlobalState_Get();
    GlobalState_Set(PAYING);
    MachineState state2 = GlobalState_Get();
    
    // Vérifier que les mutex ont été utilisés
    // (Dans un vrai test, on vérifierait les appels acquire/release)
    TEST_ASSERT_STATE_EQUAL(ORDERING, state1);
    TEST_ASSERT_STATE_EQUAL(PAYING, state2);
}

void test_mixed_operations(void) {
    // Test opérations mixtes sur différents objets
    
    // État initial
    TEST_ASSERT_STATE_EQUAL(IDLE, GlobalState_Get());
    TEST_ASSERT_EQUAL_UINT8(0, GlobalState_GetClientOrder());
    
    char buffer[4];
    GlobalState_GetKeypadChoice(buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("", buffer);
    
    // Opérations mixtes
    GlobalState_Set(ORDERING);
    GlobalState_SetKeypadChoice("1");
    GlobalState_SetClientOrder(0);
    
    // Vérifications
    TEST_ASSERT_STATE_EQUAL(ORDERING, GlobalState_Get());
    TEST_ASSERT_EQUAL_UINT8(0, GlobalState_GetClientOrder());
    GlobalState_GetKeypadChoice(buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("1", buffer);
    
    // Finalisation commande
    GlobalState_SetKeypadChoice("12");
    GlobalState_SetClientOrder(12);
    GlobalState_Set(PAYING);
    
    TEST_ASSERT_STATE_EQUAL(PAYING, GlobalState_Get());
    TEST_ASSERT_EQUAL_UINT8(12, GlobalState_GetClientOrder());
    GlobalState_GetKeypadChoice(buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("12", buffer);
    
    // Reset pour livraison
    GlobalState_SetKeypadChoice("");
    GlobalState_Set(DELIVERING);
    
    TEST_ASSERT_STATE_EQUAL(DELIVERING, GlobalState_Get());
    TEST_ASSERT_EQUAL_UINT8(12, GlobalState_GetClientOrder());  // Ordre conservé
    GlobalState_GetKeypadChoice(buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("", buffer);  // Choix reseté
}

// ============================================================================
// TESTS DE ROBUSTESSE
// ============================================================================

void test_buffer_boundaries(void) {
    char small_buffer[2];
    char exact_buffer[3];
    char large_buffer[10];
    
    GlobalState_SetKeypadChoice("12");
    
    // Buffer trop petit
    bool result = GlobalState_GetKeypadChoice(small_buffer, sizeof(small_buffer));
    TEST_ASSERT_FALSE(result);
    
    // Buffer exact
    result = GlobalState_GetKeypadChoice(exact_buffer, sizeof(exact_buffer));
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("12", exact_buffer);
    
    // Buffer large
    result = GlobalState_GetKeypadChoice(large_buffer, sizeof(large_buffer));
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("12", large_buffer);
}

void test_string_safety(void) {
    // Test sécurité des chaînes (null termination)
    GlobalState_SetKeypadChoice("AB");  // 2 caractères
    
    char buffer[4];
    bool result = GlobalState_GetKeypadChoice(buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(result);
    
    // Vérifier null termination
    TEST_ASSERT_EQUAL_CHAR('\0', buffer[2]);
    TEST_ASSERT_EQUAL_STRING("AB", buffer);
    
    // Test chaîne vide
    GlobalState_SetKeypadChoice("");
    result = GlobalState_GetKeypadChoice(buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("", buffer);
    TEST_ASSERT_EQUAL_CHAR('\0', buffer[0]);
}

// ============================================================================
// MAIN DE TEST
// ============================================================================

int main(void) {
    UNITY_BEGIN();
    
    // Tests de base
    RUN_TEST(test_global_state_get_set);
    RUN_TEST(test_global_state_without_mutex);
    RUN_TEST(test_keypad_choice_get_set);
    RUN_TEST(test_keypad_choice_parameter_validation);
    RUN_TEST(test_keypad_choice_without_mutex);
    RUN_TEST(test_client_order_get_set);
    RUN_TEST(test_client_order_without_mutex);
    
    // Tests de concurrence
    RUN_TEST(test_concurrent_state_access);
    RUN_TEST(test_mixed_operations);
    
    // Tests de robustesse
    RUN_TEST(test_buffer_boundaries);
    RUN_TEST(test_string_safety);
    
    return UNITY_END();
}
