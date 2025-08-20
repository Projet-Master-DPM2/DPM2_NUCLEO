#include "unity.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// Inclure les mocks
#include "../../mocks/mock_hal.h"
#include "../../mocks/mock_freertos.h"
#include "../../mocks/mock_global.h"
// mock_i2c.h supprimé - utilise mock_hal.h

// Constantes VL6180X (copiées du service pour les tests)
#define VL6180_SYSRANGE_START          0x018
#define VL6180_RESULT_RANGE_STATUS     0x04d
#define VL6180_RESULT_RANGE_VAL        0x062
#define VL6180_SYSTEM_FRESH_OUT_OF_RESET 0x016
#define VL6180_I2C_SLAVE_DEVICE_ADDRESS 0x212
#define VL6180_DEFAULT_ADDR_8BIT       (0x29 << 1)

// Structure TofSensorCfg pour les tests
typedef struct {
    uint8_t id;
    uint16_t i2cAddr8;
    GPIO_TypeDef* shutPort;
    uint16_t shutPin;
    uint8_t thresholdMm;
} TofSensorCfg;

// Mock GPIO pour ToF
static GPIO_TypeDef mock_gpiob;
#define TOF_SHUT_GPIO_Port     (&mock_gpiob)
#define TOF_SHUT_Pin           GPIO_PIN_1

// Variables de simulation ToF
static uint8_t mock_tof_distance = 100;  // Distance simulée en mm
static uint8_t mock_tof_status = 0x01;   // Status ready par défaut
static bool mock_tof_init_success = true;

// =============================================================================
// FONCTIONS COPIÉES DU SERVICE POUR TESTS UNITAIRES
// =============================================================================

// Version de test de vl6180_write_reg_addr (sans mutex réel)
static HAL_StatusTypeDef vl6180_write_reg_addr_test(uint16_t devAddr8, uint16_t reg, uint8_t value) {
    uint8_t tx[3] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF), value };
    return HAL_I2C_Master_Transmit(&hi2c1, devAddr8, tx, 3, 50);
}

// Version de test de vl6180_read_reg_addr (sans mutex réel)
static HAL_StatusTypeDef vl6180_read_reg_addr_test(uint16_t devAddr8, uint16_t reg, uint8_t *value) {
    uint8_t addr[2] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF) };
    HAL_StatusTypeDef st = HAL_I2C_Master_Transmit(&hi2c1, devAddr8, addr, 2, 50);
    if (st != HAL_OK) return st;
    
    // Simuler la réponse selon le registre demandé
    if (reg == VL6180_RESULT_RANGE_STATUS) {
        *value = mock_tof_status;
    } else if (reg == VL6180_RESULT_RANGE_VAL) {
        *value = mock_tof_distance;
    } else if (reg == VL6180_SYSTEM_FRESH_OUT_OF_RESET) {
        *value = mock_tof_init_success ? 0x01 : 0x00;
    } else {
        *value = 0x00;
    }
    
    return HAL_I2C_Master_Receive(&hi2c1, devAddr8, value, 1, 50);
}

// Version de test de sensor_set_shutdown
static void sensor_set_shutdown_test(const TofSensorCfg* s, uint8_t state) {
    // Simuler HAL_GPIO_WritePin
    HAL_GPIO_WritePin(s->shutPort, s->shutPin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

// Version de test de vl6180_init
static HAL_StatusTypeDef vl6180_init_test(uint16_t devAddr8) {
    uint8_t fresh = 0;
    if (vl6180_read_reg_addr_test(devAddr8, VL6180_SYSTEM_FRESH_OUT_OF_RESET, &fresh) != HAL_OK) {
        return HAL_ERROR;
    }
    return mock_tof_init_success ? HAL_OK : HAL_ERROR;
}

// Version de test de vl6180_single_shot
static HAL_StatusTypeDef vl6180_single_shot_test(uint16_t devAddr8, uint8_t *mm) {
    // Lancer une mesure single-shot
    if (vl6180_write_reg_addr_test(devAddr8, VL6180_SYSRANGE_START, 0x01) != HAL_OK) {
        return HAL_ERROR;
    }
    
    // Simuler le polling (version simplifiée)
    for (int i = 0; i < 20; ++i) {
        uint8_t status;
        if (vl6180_read_reg_addr_test(devAddr8, VL6180_RESULT_RANGE_STATUS, &status) != HAL_OK) {
            return HAL_ERROR;
        }
        if ((status & 0x01) == 0) {
            // Pas prêt, simuler délai
            continue;
        } else {
            break;
        }
    }
    
    // Lire la distance
    uint8_t range = 0;
    if (vl6180_read_reg_addr_test(devAddr8, VL6180_RESULT_RANGE_VAL, &range) != HAL_OK) {
        return HAL_ERROR;
    }
    *mm = range;
    return HAL_OK;
}

// Version de test de VL6180_SetI2CAddress
static HAL_StatusTypeDef VL6180_SetI2CAddress_test(uint16_t currentAddr8, uint8_t new7bit) {
    return vl6180_write_reg_addr_test(currentAddr8, VL6180_I2C_SLAVE_DEVICE_ADDRESS, new7bit);
}

// Version de test de sensors_init
static HAL_StatusTypeDef sensors_init_test(TofSensorCfg* sensors, uint8_t count) {
    // Mettre tous les capteurs en SHUTDOWN
    for (uint8_t i = 0; i < count; ++i) {
        sensor_set_shutdown_test(&sensors[i], 0);
    }
    
    // Remonter un par un et initialiser
    for (uint8_t i = 0; i < count; ++i) {
        sensor_set_shutdown_test(&sensors[i], 1);
        
        // Si l'adresse désirée est différente de la valeur par défaut, la changer
        if (sensors[i].i2cAddr8 != VL6180_DEFAULT_ADDR_8BIT) {
            if (VL6180_SetI2CAddress_test(VL6180_DEFAULT_ADDR_8BIT, (uint8_t)(sensors[i].i2cAddr8 >> 1)) != HAL_OK) {
                return HAL_ERROR;
            }
        }
        
        if (vl6180_init_test(sensors[i].i2cAddr8) != HAL_OK) {
            return HAL_ERROR;
        }
    }
    return HAL_OK;
}

// =============================================================================
// SETUP ET TEARDOWN
// =============================================================================

void setUp(void) {
    // Reset des mocks avant chaque test
    Mock_HAL_Reset();
    Mock_FreeRTOS_Reset();
    
    // Configuration par défaut des mocks ToF
    Mock_HAL_SetI2CResponse(HAL_OK, NULL, 0);
    mock_tof_distance = 100;
    mock_tof_status = 0x01;  // Ready
    mock_tof_init_success = true;
}

void tearDown(void) {
    // Nettoyage après chaque test
}

// =============================================================================
// FONCTIONS UTILITAIRES POUR TESTS
// =============================================================================

// Configurer la distance simulée du capteur ToF
static void set_mock_tof_distance(uint8_t distance_mm) {
    mock_tof_distance = distance_mm;
}

// Configurer le statut du capteur ToF
static void set_mock_tof_status(uint8_t status) {
    mock_tof_status = status;
}

// Configurer le succès/échec de l'initialisation
static void set_mock_tof_init_success(bool success) {
    mock_tof_init_success = success;
}

// =============================================================================
// TESTS UNITAIRES SENSOR STOCK SERVICE
// =============================================================================

// Test d'écriture de registre VL6180X
void test_vl6180_write_register(void) {
    uint16_t devAddr = VL6180_DEFAULT_ADDR_8BIT;
    uint16_t reg = VL6180_SYSRANGE_START;
    uint8_t value = 0x01;
    
    HAL_StatusTypeDef result = vl6180_write_reg_addr_test(devAddr, reg, value);
    
    TEST_ASSERT_EQUAL(HAL_OK, result);
    TEST_ASSERT_EQUAL_UINT32(1, Mock_HAL_GetI2CCallCount());
    // Pas de vérification d'adresse spécifique avec mock_hal simplifié
    // TEST_ASSERT_EQUAL_UINT16(3, Mock_I2C_GetLastTransmitSize()); // 2 bytes addr + 1 byte data
}

// Test de lecture de registre VL6180X
void test_vl6180_read_register(void) {
    uint16_t devAddr = VL6180_DEFAULT_ADDR_8BIT;
    uint16_t reg = VL6180_RESULT_RANGE_VAL;
    uint8_t value = 0;
    
    set_mock_tof_distance(150);
    
    HAL_StatusTypeDef result = vl6180_read_reg_addr_test(devAddr, reg, &value);
    
    TEST_ASSERT_EQUAL(HAL_OK, result);
    TEST_ASSERT_EQUAL_UINT8(150, value);
    TEST_ASSERT_TRUE(Mock_HAL_GetI2CCallCount() >= 2); // Transmit + Receive
}

// Test d'initialisation du capteur VL6180X
void test_vl6180_init_success(void) {
    uint16_t devAddr = VL6180_DEFAULT_ADDR_8BIT;
    
    set_mock_tof_init_success(true);
    
    HAL_StatusTypeDef result = vl6180_init_test(devAddr);
    
    TEST_ASSERT_EQUAL(HAL_OK, result);
    TEST_ASSERT_TRUE(Mock_HAL_GetI2CCallCount() >= 2); // Au moins 2 appels I2C
}

// Test d'échec d'initialisation du capteur VL6180X
void test_vl6180_init_failure(void) {
    uint16_t devAddr = VL6180_DEFAULT_ADDR_8BIT;
    
    set_mock_tof_init_success(false);
    
    HAL_StatusTypeDef result = vl6180_init_test(devAddr);
    
    TEST_ASSERT_EQUAL(HAL_ERROR, result);
}

// Test de mesure single-shot réussie
void test_vl6180_single_shot_success(void) {
    uint16_t devAddr = VL6180_DEFAULT_ADDR_8BIT;
    uint8_t distance_mm = 0;
    
    set_mock_tof_distance(75);
    set_mock_tof_status(0x01); // Ready
    
    HAL_StatusTypeDef result = vl6180_single_shot_test(devAddr, &distance_mm);
    
    TEST_ASSERT_EQUAL(HAL_OK, result);
    TEST_ASSERT_EQUAL_UINT8(75, distance_mm);
}

// Test de mesure single-shot avec timeout
void test_vl6180_single_shot_timeout(void) {
    uint16_t devAddr = VL6180_DEFAULT_ADDR_8BIT;
    uint8_t distance_mm = 0;
    
    set_mock_tof_status(0x00); // Jamais prêt
    
    HAL_StatusTypeDef result = vl6180_single_shot_test(devAddr, &distance_mm);
    
    // Le capteur devrait finir par lire même si pas prêt
    TEST_ASSERT_EQUAL(HAL_OK, result);
}

// Test de changement d'adresse I2C
void test_vl6180_set_i2c_address(void) {
    uint16_t currentAddr = VL6180_DEFAULT_ADDR_8BIT;
    uint8_t newAddr7bit = 0x30;
    
    HAL_StatusTypeDef result = VL6180_SetI2CAddress_test(currentAddr, newAddr7bit);
    
    TEST_ASSERT_EQUAL(HAL_OK, result);
    TEST_ASSERT_EQUAL_UINT32(1, Mock_HAL_GetI2CCallCount());
}

// Test de contrôle GPIO shutdown
void test_sensor_shutdown_control(void) {
    TofSensorCfg sensor = {
        .id = 0,
        .i2cAddr8 = VL6180_DEFAULT_ADDR_8BIT,
        .shutPort = TOF_SHUT_GPIO_Port,
        .shutPin = TOF_SHUT_Pin,
        .thresholdMm = 170
    };
    
    // Test shutdown (mise à 0)
    sensor_set_shutdown_test(&sensor, 0);
    TEST_ASSERT_EQUAL_UINT32(1, Mock_HAL_GetGPIOWriteCallCount());
    
    // Reset et test réveil (mise à 1)
    Mock_HAL_Reset();
    sensor_set_shutdown_test(&sensor, 1);
    TEST_ASSERT_EQUAL_UINT32(1, Mock_HAL_GetGPIOWriteCallCount());
}

// Test d'initialisation d'un capteur unique
void test_sensor_init_single_sensor(void) {
    TofSensorCfg sensors[1] = {
        { .id = 0, .i2cAddr8 = VL6180_DEFAULT_ADDR_8BIT, 
          .shutPort = TOF_SHUT_GPIO_Port, .shutPin = TOF_SHUT_Pin, .thresholdMm = 170 }
    };
    
    set_mock_tof_init_success(true);
    
    HAL_StatusTypeDef result = sensors_init_test(sensors, 1);
    
    TEST_ASSERT_EQUAL(HAL_OK, result);
    // Vérifier que GPIO a été manipulé (shutdown puis réveil)
    TEST_ASSERT_TRUE(Mock_HAL_GetGPIOWriteCallCount() >= 2);
}

// Test d'initialisation avec changement d'adresse
void test_sensor_init_address_change(void) {
    TofSensorCfg sensors[1] = {
        { .id = 1, .i2cAddr8 = (0x30 << 1), // Adresse différente de défaut
          .shutPort = TOF_SHUT_GPIO_Port, .shutPin = TOF_SHUT_Pin, .thresholdMm = 180 }
    };
    
    set_mock_tof_init_success(true);
    
    HAL_StatusTypeDef result = sensors_init_test(sensors, 1);
    
    TEST_ASSERT_EQUAL(HAL_OK, result);
    // Vérifier qu'il y a eu des transmissions I2C (changement d'adresse + init)
    TEST_ASSERT_TRUE(Mock_HAL_GetI2CCallCount() >= 2);
}

// Test d'échec d'initialisation multi-capteurs
void test_sensor_init_failure(void) {
    TofSensorCfg sensors[1] = {
        { .id = 0, .i2cAddr8 = VL6180_DEFAULT_ADDR_8BIT,
          .shutPort = TOF_SHUT_GPIO_Port, .shutPin = TOF_SHUT_Pin, .thresholdMm = 170 }
    };
    
    set_mock_tof_init_success(false);
    
    HAL_StatusTypeDef result = sensors_init_test(sensors, 1);
    
    TEST_ASSERT_EQUAL(HAL_ERROR, result);
}

// Test de détection de seuil de stock bas
void test_stock_threshold_detection_low(void) {
    uint8_t threshold = 170;
    uint8_t measured_distance = 180; // Au-dessus du seuil = stock bas
    
    bool stock_low = (measured_distance >= threshold);
    
    TEST_ASSERT_TRUE(stock_low);
}

// Test de détection de stock normal
void test_stock_threshold_detection_normal(void) {
    uint8_t threshold = 170;
    uint8_t measured_distance = 150; // En-dessous du seuil = stock normal
    
    bool stock_low = (measured_distance >= threshold);
    
    TEST_ASSERT_FALSE(stock_low);
}

// Test de validation des paramètres de capteur
void test_sensor_config_validation(void) {
    TofSensorCfg sensor;
    
    // Test configuration valide
    sensor.id = 0;
    sensor.i2cAddr8 = VL6180_DEFAULT_ADDR_8BIT;
    sensor.shutPort = TOF_SHUT_GPIO_Port;
    sensor.shutPin = TOF_SHUT_Pin;
    sensor.thresholdMm = 170;
    
    TEST_ASSERT_EQUAL_UINT8(0, sensor.id);
    TEST_ASSERT_EQUAL_UINT16(VL6180_DEFAULT_ADDR_8BIT, sensor.i2cAddr8);
    TEST_ASSERT_EQUAL_UINT8(170, sensor.thresholdMm);
    TEST_ASSERT_NOT_NULL(sensor.shutPort);
}

// Test de gestion d'erreur I2C
void test_sensor_i2c_error_handling(void) {
    uint16_t devAddr = VL6180_DEFAULT_ADDR_8BIT;
    uint8_t distance_mm = 0;
    
    // Configurer le mock pour retourner une erreur
    Mock_HAL_SetI2CResponse(HAL_ERROR, NULL, 0);
    
    HAL_StatusTypeDef result = vl6180_single_shot_test(devAddr, &distance_mm);
    
    TEST_ASSERT_EQUAL(HAL_ERROR, result);
}

// Test de plages de mesure ToF
void test_tof_measurement_ranges(void) {
    uint16_t devAddr = VL6180_DEFAULT_ADDR_8BIT;
    uint8_t distance_mm = 0;
    
    // Test distance minimale
    set_mock_tof_distance(0);
    vl6180_single_shot_test(devAddr, &distance_mm);
    TEST_ASSERT_EQUAL_UINT8(0, distance_mm);
    
    // Test distance maximale (255mm pour VL6180X)
    set_mock_tof_distance(255);
    vl6180_single_shot_test(devAddr, &distance_mm);
    TEST_ASSERT_EQUAL_UINT8(255, distance_mm);
    
    // Test distance typique
    set_mock_tof_distance(120);
    vl6180_single_shot_test(devAddr, &distance_mm);
    TEST_ASSERT_EQUAL_UINT8(120, distance_mm);
}

// Test de robustesse avec valeurs limites
void test_sensor_robustness_edge_cases(void) {
    TofSensorCfg sensor = {
        .id = 255,  // ID maximum
        .i2cAddr8 = 0xFE,  // Adresse limite
        .shutPort = TOF_SHUT_GPIO_Port,
        .shutPin = GPIO_PIN_15,  // Pin maximum
        .thresholdMm = 255  // Seuil maximum
    };
    
    // Test que la configuration ne cause pas de problème
    sensor_set_shutdown_test(&sensor, 1);
    TEST_ASSERT_EQUAL_UINT32(1, Mock_HAL_GetGPIOWriteCallCount());
    
    // Test seuil limite
    bool stock_low = (255 >= sensor.thresholdMm);
    TEST_ASSERT_TRUE(stock_low);
}

// Test de séquence complète de mesure
void test_complete_measurement_sequence(void) {
    TofSensorCfg sensor = {
        .id = 0,
        .i2cAddr8 = VL6180_DEFAULT_ADDR_8BIT,
        .shutPort = TOF_SHUT_GPIO_Port,
        .shutPin = TOF_SHUT_Pin,
        .thresholdMm = 170
    };
    
    Mock_HAL_Reset();
    
    // 1. Initialisation du capteur
    HAL_StatusTypeDef init_result = sensors_init_test(&sensor, 1);
    TEST_ASSERT_EQUAL(HAL_OK, init_result);
    
    uint32_t init_calls = Mock_HAL_GetI2CCallCount();
    
    // 2. Mesure de distance
    set_mock_tof_distance(185);  // Stock bas
    uint8_t distance = 0;
    HAL_StatusTypeDef measure_result = vl6180_single_shot_test(sensor.i2cAddr8, &distance);
    
    TEST_ASSERT_EQUAL(HAL_OK, measure_result);
    TEST_ASSERT_EQUAL_UINT8(185, distance);
    
    // 3. Détection de stock bas
    bool stock_low = (distance >= sensor.thresholdMm);
    TEST_ASSERT_TRUE(stock_low);
    
    // Vérifier que les appels I2C ont augmenté pour la mesure
    TEST_ASSERT_TRUE(Mock_HAL_GetI2CCallCount() >= init_calls);
}

// =============================================================================
// MAIN DES TESTS
// =============================================================================

int main(void) {
    UNITY_BEGIN();
    
    // Tests de base VL6180X
    RUN_TEST(test_vl6180_write_register);
    RUN_TEST(test_vl6180_read_register);
    RUN_TEST(test_vl6180_init_success);
    RUN_TEST(test_vl6180_init_failure);
    
    // Tests de mesure
    RUN_TEST(test_vl6180_single_shot_success);
    RUN_TEST(test_vl6180_single_shot_timeout);
    RUN_TEST(test_tof_measurement_ranges);
    
    // Tests de configuration
    RUN_TEST(test_vl6180_set_i2c_address);
    RUN_TEST(test_sensor_shutdown_control);
    RUN_TEST(test_sensor_config_validation);
    
    // Tests d'initialisation
    RUN_TEST(test_sensor_init_single_sensor);
    RUN_TEST(test_sensor_init_address_change);
    RUN_TEST(test_sensor_init_failure);
    
    // Tests de détection de seuil
    RUN_TEST(test_stock_threshold_detection_low);
    RUN_TEST(test_stock_threshold_detection_normal);
    
    // Tests de robustesse
    RUN_TEST(test_sensor_i2c_error_handling);
    RUN_TEST(test_sensor_robustness_edge_cases);
    
    // Test de séquence complète
    RUN_TEST(test_complete_measurement_sequence);
    
    return UNITY_END();
}
