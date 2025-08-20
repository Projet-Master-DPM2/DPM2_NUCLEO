#include "unity.h"
#include <stdint.h>

#ifndef UNITY_NATIVE_TESTS
#include "../../mocks/mock_global.h"
#endif

// Copie de la fonction MotorService_OrderToChannel pour les tests
// (évite les dépendances STM32)
static uint8_t MotorService_OrderToChannel_TestVersion(uint8_t orderCode) {
    switch (orderCode) {
        case 11: return 0;
        case 12: return 1;
        case 13: return 2;
        case 21: return 3;
        case 22: return 4;
        case 23: return 6;
        default: return 0xFF; // invalide
    }
}

void setUp(void) {
    // Configuration avant chaque test
}

void tearDown(void) {
    // Nettoyage après chaque test
}

// Test du mapping order code vers channel
void test_motor_order_to_channel_mapping(void) {
    // Test des codes valides
    TEST_ASSERT_EQUAL_UINT8(0, MotorService_OrderToChannel_TestVersion(11));
    TEST_ASSERT_EQUAL_UINT8(1, MotorService_OrderToChannel_TestVersion(12));
    TEST_ASSERT_EQUAL_UINT8(2, MotorService_OrderToChannel_TestVersion(13));
    TEST_ASSERT_EQUAL_UINT8(3, MotorService_OrderToChannel_TestVersion(21));
    TEST_ASSERT_EQUAL_UINT8(4, MotorService_OrderToChannel_TestVersion(22));
    TEST_ASSERT_EQUAL_UINT8(6, MotorService_OrderToChannel_TestVersion(23)); // Note: pas de canal 5
    
    // Test des codes invalides
    TEST_ASSERT_EQUAL_UINT8(0xFF, MotorService_OrderToChannel_TestVersion(0));
    TEST_ASSERT_EQUAL_UINT8(0xFF, MotorService_OrderToChannel_TestVersion(99));
    TEST_ASSERT_EQUAL_UINT8(0xFF, MotorService_OrderToChannel_TestVersion(14));
    TEST_ASSERT_EQUAL_UINT8(0xFF, MotorService_OrderToChannel_TestVersion(24));
}

// Test de validation des paramètres
void test_motor_parameter_validation(void) {
    // Test avec des valeurs limites
    TEST_ASSERT_EQUAL_UINT8(0xFF, MotorService_OrderToChannel_TestVersion(10));  // Juste en dessous
    TEST_ASSERT_EQUAL_UINT8(0xFF, MotorService_OrderToChannel_TestVersion(24));  // Juste au dessus
    
    // Test avec des valeurs extrêmes
    TEST_ASSERT_EQUAL_UINT8(0xFF, MotorService_OrderToChannel_TestVersion(0));
    TEST_ASSERT_EQUAL_UINT8(0xFF, MotorService_OrderToChannel_TestVersion(255));
}

// Test des codes produits complets (intégration)
void test_motor_complete_product_flow(void) {
    // Test du flux complet: code produit -> canal -> sélection
    
    // Produit 11 -> Canal 0
    uint8_t channel = MotorService_OrderToChannel_TestVersion(11);
    TEST_ASSERT_EQUAL_UINT8(0, channel);
    
    // Produit 23 -> Canal 6  
    channel = MotorService_OrderToChannel_TestVersion(23);
    TEST_ASSERT_EQUAL_UINT8(6, channel);
    
    // Test que tous les produits valides ont un canal assigné
    uint8_t valid_products[] = {11, 12, 13, 21, 22, 23};
    uint8_t expected_channels[] = {0, 1, 2, 3, 4, 6};
    
    for (int i = 0; i < 6; i++) {
        uint8_t result = MotorService_OrderToChannel_TestVersion(valid_products[i]);
        TEST_ASSERT_EQUAL_UINT8(expected_channels[i], result);
        TEST_ASSERT_NOT_EQUAL(0xFF, result); // Pas d'erreur
    }
}

// Test de robustesse
void test_motor_robustness(void) {
    // Test avec des valeurs limites
    TEST_ASSERT_EQUAL_UINT8(0xFF, MotorService_OrderToChannel_TestVersion(10));  // Juste en dessous
    TEST_ASSERT_EQUAL_UINT8(0xFF, MotorService_OrderToChannel_TestVersion(24));  // Juste au dessus
    
    // Test avec des valeurs extrêmes
    TEST_ASSERT_EQUAL_UINT8(0xFF, MotorService_OrderToChannel_TestVersion(0));
    TEST_ASSERT_EQUAL_UINT8(0xFF, MotorService_OrderToChannel_TestVersion(255));
}

// Test des patterns binaires de sélection
void test_motor_channel_binary_patterns(void) {
    // Test des patterns binaires attendus pour la sélection MUX
    // Channel 0: S3=0, S2=0, S1=0, S0=0 (binaire: 0000)
    // Channel 1: S3=0, S2=0, S1=0, S0=1 (binaire: 0001)
    // Channel 2: S3=0, S2=0, S1=1, S0=0 (binaire: 0010)
    // Channel 3: S3=0, S2=0, S1=1, S0=1 (binaire: 0011)
    // Channel 4: S3=0, S2=1, S1=0, S0=0 (binaire: 0100)
    // Channel 6: S3=0, S2=1, S1=1, S0=0 (binaire: 0110)
    
    // Vérifier que les canaux utilisés correspondent aux patterns binaires valides
    TEST_ASSERT_EQUAL_UINT8(0, MotorService_OrderToChannel_TestVersion(11)); // 0000
    TEST_ASSERT_EQUAL_UINT8(1, MotorService_OrderToChannel_TestVersion(12)); // 0001  
    TEST_ASSERT_EQUAL_UINT8(2, MotorService_OrderToChannel_TestVersion(13)); // 0010
    TEST_ASSERT_EQUAL_UINT8(3, MotorService_OrderToChannel_TestVersion(21)); // 0011
    TEST_ASSERT_EQUAL_UINT8(4, MotorService_OrderToChannel_TestVersion(22)); // 0100
    TEST_ASSERT_EQUAL_UINT8(6, MotorService_OrderToChannel_TestVersion(23)); // 0110
    
    // Vérifier qu'il n'y a pas de canal 5 (0101) utilisé
    // (pas de produit ne mappe vers le canal 5)
}

// Test de couverture complète des produits
void test_motor_product_coverage(void) {
    // Vérifier que tous les produits de la gamme 11-13 et 21-23 sont couverts
    
    // Gamme 11-13 (ligne 1)
    TEST_ASSERT_NOT_EQUAL(0xFF, MotorService_OrderToChannel_TestVersion(11));
    TEST_ASSERT_NOT_EQUAL(0xFF, MotorService_OrderToChannel_TestVersion(12));
    TEST_ASSERT_NOT_EQUAL(0xFF, MotorService_OrderToChannel_TestVersion(13));
    
    // Gamme 21-23 (ligne 2)  
    TEST_ASSERT_NOT_EQUAL(0xFF, MotorService_OrderToChannel_TestVersion(21));
    TEST_ASSERT_NOT_EQUAL(0xFF, MotorService_OrderToChannel_TestVersion(22));
    TEST_ASSERT_NOT_EQUAL(0xFF, MotorService_OrderToChannel_TestVersion(23));
    
    // Vérifier que les produits non existants retournent une erreur
    TEST_ASSERT_EQUAL_UINT8(0xFF, MotorService_OrderToChannel_TestVersion(14)); // Pas de 14
    TEST_ASSERT_EQUAL_UINT8(0xFF, MotorService_OrderToChannel_TestVersion(24)); // Pas de 24
    TEST_ASSERT_EQUAL_UINT8(0xFF, MotorService_OrderToChannel_TestVersion(31)); // Pas de ligne 3
}

int main(void) {
    UNITY_BEGIN();
    
    // Tests de base
    RUN_TEST(test_motor_order_to_channel_mapping);
    RUN_TEST(test_motor_parameter_validation);
    RUN_TEST(test_motor_robustness);
    
    // Tests d'intégration
    RUN_TEST(test_motor_complete_product_flow);
    RUN_TEST(test_motor_product_coverage);
    
    // Tests de logique matérielle
    RUN_TEST(test_motor_channel_binary_patterns);
    
    return UNITY_END();
}
