#include "sensor_stock_service.h"
#include "orchestrator.h"
#include <stdio.h>
#include "global.h"

// Registres clés du VL6180X (voir AN ST)
#define VL6180_SYSRANGE_START          0x018
#define VL6180_RESULT_RANGE_STATUS     0x04d
#define VL6180_RESULT_RANGE_VAL        0x062
#define VL6180_SYSTEM_FRESH_OUT_OF_RESET 0x016
#define VL6180_I2C_SLAVE_DEVICE_ADDRESS 0x212

static HAL_StatusTypeDef vl6180_write_reg_addr(uint16_t devAddr8, uint16_t reg, uint8_t value) {
    uint8_t tx[3] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF), value };
    osMutexAcquire(i2c1Mutex, osWaitForever);
    HAL_StatusTypeDef st = HAL_I2C_Master_Transmit(&hi2c1, devAddr8, tx, 3, 50);
    osMutexRelease(i2c1Mutex);
    return st;
}

static HAL_StatusTypeDef vl6180_read_reg_addr(uint16_t devAddr8, uint16_t reg, uint8_t *value) {
    uint8_t addr[2] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF) };
    osMutexAcquire(i2c1Mutex, osWaitForever);
    HAL_StatusTypeDef st = HAL_I2C_Master_Transmit(&hi2c1, devAddr8, addr, 2, 50);
    if (st != HAL_OK) return st;
    st = HAL_I2C_Master_Receive(&hi2c1, devAddr8, value, 1, 50);
    osMutexRelease(i2c1Mutex);
    return st;
}

static void sensor_set_shutdown(const TofSensorCfg* s, uint8_t state) {
    HAL_GPIO_WritePin(s->shutPort, s->shutPin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static HAL_StatusTypeDef vl6180_init(uint16_t devAddr8) {
    // Vérifier le flag Fresh out of reset (optionnel)
    uint8_t fresh = 0;
    if (vl6180_read_reg_addr(devAddr8, VL6180_SYSTEM_FRESH_OUT_OF_RESET, &fresh) != HAL_OK) return HAL_ERROR;
    // Quelques writes de tuning peuvent être nécessaires selon AN (omises ici pour simplicité)
    return HAL_OK;
}

static HAL_StatusTypeDef vl6180_single_shot(uint16_t devAddr8, uint8_t *mm) {
    // Lancer une mesure single-shot
    if (vl6180_write_reg_addr(devAddr8, VL6180_SYSRANGE_START, 0x01) != HAL_OK) return HAL_ERROR;
    // Polling simple sur RESULT_RANGE_STATUS bit 0 (ready)
    for (int i = 0; i < 20; ++i) {
        uint8_t status;
        if (vl6180_read_reg_addr(devAddr8, VL6180_RESULT_RANGE_STATUS, &status) != HAL_OK) return HAL_ERROR;
        if ((status & 0x01) == 0) {
            osDelay(5);
        } else {
            break;
        }
    }
    // Lire la distance
    uint8_t range = 0;
    if (vl6180_read_reg_addr(devAddr8, VL6180_RESULT_RANGE_VAL, &range) != HAL_OK) return HAL_ERROR;
    *mm = range;
    return HAL_OK;
}

HAL_StatusTypeDef VL6180_SetI2CAddress(I2C_HandleTypeDef* hi2c, uint16_t currentAddr8, uint8_t new7bit) {
    // Ecriture du nouveau 7-bit dans le registre 0x021
    return vl6180_write_reg_addr(currentAddr8, VL6180_I2C_SLAVE_DEVICE_ADDRESS, new7bit);
}

static HAL_StatusTypeDef sensors_init(TofSensorCfg* sensors, uint8_t count) {
    // Mettre tous les capteurs en SHUTDOWN
    for (uint8_t i = 0; i < count; ++i) {
        sensor_set_shutdown(&sensors[i], 0);
    }
    osDelay(2);
    // Remonter un par un, changer l'adresse si nécessaire, puis initialiser
    for (uint8_t i = 0; i < count; ++i) {
        sensor_set_shutdown(&sensors[i], 1);
        osDelay(10);
        // Si l'adresse désirée est différente de la valeur par défaut, la changer
        if (sensors[i].i2cAddr8 != VL6180_DEFAULT_ADDR_8BIT) {
            if (VL6180_SetI2CAddress(&hi2c1, VL6180_DEFAULT_ADDR_8BIT, (uint8_t)(sensors[i].i2cAddr8 >> 1)) != HAL_OK) {
                printf("TOF[%d] set addr failed\r\n", sensors[i].id);
                return HAL_ERROR;
            }
            osDelay(2);
        }
        if (vl6180_init(sensors[i].i2cAddr8) != HAL_OK) {
            printf("TOF[%d] init failed\r\n", sensors[i].id);
            return HAL_ERROR;
        }
    }
    return HAL_OK;
}

void StartTaskSensorStock(void *argument) {
    printf("\r\nSensorStock Task started\r\n");
    // Configuration des 5 capteurs ToF avec leurs broches SHUT respectives
    TofSensorCfg sensors[5] = {
        { .id = 0, .i2cAddr8 = (0x29 << 1), .shutPort = TOF_SHUT_1_GPIO_Port, .shutPin = TOF_SHUT_1_Pin, .thresholdMm = 170 },
        { .id = 1, .i2cAddr8 = (0x2A << 1), .shutPort = TOF_SHUT_2_GPIO_Port, .shutPin = TOF_SHUT_2_Pin, .thresholdMm = 170 },
        { .id = 2, .i2cAddr8 = (0x2B << 1), .shutPort = TOF_SHUT_3_GPIO_Port, .shutPin = TOF_SHUT_3_Pin, .thresholdMm = 170 },
        { .id = 3, .i2cAddr8 = (0x2C << 1), .shutPort = TOF_SHUT_4_GPIO_Port, .shutPin = TOF_SHUT_4_Pin, .thresholdMm = 170 },
        { .id = 4, .i2cAddr8 = (0x2D << 1), .shutPort = TOF_SHUT_5_GPIO_Port, .shutPin = TOF_SHUT_5_Pin, .thresholdMm = 170 }
    };

    if (sensors_init(sensors, 5) != HAL_OK) {
        printf("VL6180 init error\r\n");
    } else {
        printf("VL6180 ready (5 sensors)\r\n");
    }

    for (;;) {
        for (uint8_t i = 0; i < 5; ++i) {
            uint8_t mm = 0;
            if (vl6180_single_shot(sensors[i].i2cAddr8, &mm) == HAL_OK) {
                printf("TOF[%d] distance: %u mm\r\n", sensors[i].id, mm);
                if (mm >= sensors[i].thresholdMm) {
                    extern osMessageQueueId_t orchestratorEventQueueHandle;
                    OrchestratorEvent evt = { .type = ORCH_EVT_STOCK_LOW };
                    evt.data.stock.sensorId = sensors[i].id;
                    evt.data.stock.mm = mm;
                    osMessageQueuePut(orchestratorEventQueueHandle, &evt, 0, 0);
                }
            } else {
                printf("TOF[%d] read error\r\n", sensors[i].id);
            }
        }
        osDelay(200);
    }
}


