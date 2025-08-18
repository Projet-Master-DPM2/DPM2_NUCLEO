#ifndef MOCK_HAL_H
#define MOCK_HAL_H

#include <stdint.h>
#include <stdbool.h>

// Mock pour les tests natifs uniquement
#ifdef UNITY_NATIVE_TESTS

// Types HAL mockés
typedef enum {
    HAL_OK = 0,
    HAL_ERROR = 1,
    HAL_BUSY = 2,
    HAL_TIMEOUT = 3
} HAL_StatusTypeDef;

typedef enum {
    GPIO_PIN_RESET = 0,
    GPIO_PIN_SET = 1
} GPIO_PinState;

// Structures mockées
typedef struct {
    uint32_t dummy;
} GPIO_TypeDef;

typedef struct {
    uint32_t dummy;
} I2C_HandleTypeDef;

typedef struct {
    uint32_t dummy;
} UART_HandleTypeDef;

// Variables mockées
extern GPIO_TypeDef* GPIOA;
extern GPIO_TypeDef* GPIOB;
extern GPIO_TypeDef* GPIOC;
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

// Pins mockées
#define GPIO_PIN_0   (1 << 0)
#define GPIO_PIN_1   (1 << 1)
#define GPIO_PIN_5   (1 << 5)
#define GPIO_PIN_8   (1 << 8)
#define GPIO_PIN_9   (1 << 9)
#define GPIO_PIN_10  (1 << 10)
#define GPIO_PIN_13  (1 << 13)

// Fonctions HAL mockées
HAL_StatusTypeDef HAL_GPIO_ReadPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void HAL_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);

// Fonction de temps mockée
uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t Delay);

// Fonctions de contrôle des mocks
void Mock_HAL_Reset(void);
void Mock_HAL_SetTick(uint32_t tick);
void Mock_HAL_SetGpioPin(GPIO_TypeDef* port, uint16_t pin, GPIO_PinState state);
void Mock_HAL_SetI2CResponse(HAL_StatusTypeDef status, uint8_t* data, uint16_t size);
void Mock_HAL_SetUARTResponse(HAL_StatusTypeDef status);

// Vérifications des mocks
uint32_t Mock_HAL_GetI2CCallCount(void);
uint32_t Mock_HAL_GetUARTCallCount(void);
uint32_t Mock_HAL_GetGpioWriteCount(void);

#endif // UNITY_NATIVE_TESTS

#endif // MOCK_HAL_H
