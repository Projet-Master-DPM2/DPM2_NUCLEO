#ifndef SENSOR_STOCK_SERVICE_H
#define SENSOR_STOCK_SERVICE_H

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "i2c.h"

// Adresse I2C 7-bit par défaut du VL6180 = 0x29 (HAL attend l'adresse 8-bit)
// Adresse par défaut 0x29, mais support multi-capteurs: 8-bit HAL
#define VL6180_DEFAULT_ADDR_8BIT   (0x29 << 1)

typedef struct {
    uint8_t id;            // identifiant logique 0..N-1
    uint16_t i2cAddr8;     // adresse 8-bit HAL
    GPIO_TypeDef* shutPort;
    uint16_t shutPin;
    uint8_t thresholdMm;   // seuil de stock bas (distance)
} TofSensorCfg;

// Broches SHUT des capteurs ToF (5 capteurs)
// Chaque capteur a sa propre broche SHUT pour l'activation/désactivation
#define TOF_SHUT_1_GPIO_Port   GPIOB
#define TOF_SHUT_1_Pin         GPIO_PIN_2

#define TOF_SHUT_2_GPIO_Port   GPIOB
#define TOF_SHUT_2_Pin         GPIO_PIN_1

#define TOF_SHUT_3_GPIO_Port   GPIOB
#define TOF_SHUT_3_Pin         GPIO_PIN_15

#define TOF_SHUT_4_GPIO_Port   GPIOB
#define TOF_SHUT_4_Pin         GPIO_PIN_14

#define TOF_SHUT_5_GPIO_Port   GPIOB
#define TOF_SHUT_5_Pin         GPIO_PIN_13

void StartTaskSensorStock(void *argument);
HAL_StatusTypeDef VL6180_SetI2CAddress(I2C_HandleTypeDef* hi2c, uint16_t currentAddr8, uint8_t new7bit);

#endif

