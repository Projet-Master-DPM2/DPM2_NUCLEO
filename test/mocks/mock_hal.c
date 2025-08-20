#include "mock_hal.h"

#ifdef UNITY_NATIVE_TESTS

#include <stdlib.h>
#include <string.h>
#include <time.h>

// Variables globales mockées
static GPIO_TypeDef gpio_a = {0};
static GPIO_TypeDef gpio_b = {0};
static GPIO_TypeDef gpio_c = {0};

GPIO_TypeDef* GPIOA = &gpio_a;
GPIO_TypeDef* GPIOB = &gpio_b;
GPIO_TypeDef* GPIOC = &gpio_c;

I2C_HandleTypeDef hi2c1 = {0};
UART_HandleTypeDef huart1 = {0};
UART_HandleTypeDef huart2 = {0};

// État des mocks
static struct {
    uint32_t current_tick;
    GPIO_PinState gpio_states[16];  // État des pins
    HAL_StatusTypeDef i2c_response;
    uint8_t i2c_data[256];
    uint16_t i2c_data_size;
    HAL_StatusTypeDef uart_response;
    
    // Compteurs d'appels
    uint32_t i2c_call_count;
    uint32_t uart_call_count;
    uint32_t gpio_write_count;
} mock_state = {0};

// ============================================================================
// FONCTIONS HAL MOCKÉES
// ============================================================================

HAL_StatusTypeDef HAL_GPIO_ReadPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    // Convertir le pin en index (position du bit)
    int pin_index = 0;
    uint16_t pin_mask = GPIO_Pin;
    while (pin_mask > 1) {
        pin_mask >>= 1;
        pin_index++;
    }
    
    if (pin_index < 16) {
        return mock_state.gpio_states[pin_index];
    }
    return GPIO_PIN_RESET;
}

void HAL_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState) {
    mock_state.gpio_write_count++;
    
    // Convertir le pin en index
    int pin_index = 0;
    uint16_t pin_mask = GPIO_Pin;
    while (pin_mask > 1) {
        pin_mask >>= 1;
        pin_index++;
    }
    
    if (pin_index < 16) {
        mock_state.gpio_states[pin_index] = PinState;
    }
}

HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, 
                                          uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    mock_state.i2c_call_count++;
    return mock_state.i2c_response;
}

HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, 
                                         uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    mock_state.i2c_call_count++;
    
    if (mock_state.i2c_response == HAL_OK && pData && Size > 0) {
        uint16_t copy_size = (Size < mock_state.i2c_data_size) ? Size : mock_state.i2c_data_size;
        memcpy(pData, mock_state.i2c_data, copy_size);
    }
    
    return mock_state.i2c_response;
}

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, 
                                    uint16_t Size, uint32_t Timeout) {
    mock_state.uart_call_count++;
    return mock_state.uart_response;
}

HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size) {
    mock_state.uart_call_count++;
    return mock_state.uart_response;
}

uint32_t HAL_GetTick(void) {
    return mock_state.current_tick;
}

void HAL_Delay(uint32_t Delay) {
    mock_state.current_tick += Delay;
}

// ============================================================================
// FONCTIONS DE CONTRÔLE DES MOCKS
// ============================================================================

void Mock_HAL_Reset(void) {
    memset(&mock_state, 0, sizeof(mock_state));
    mock_state.i2c_response = HAL_OK;
    mock_state.uart_response = HAL_OK;
}

void Mock_HAL_SetTick(uint32_t tick) {
    mock_state.current_tick = tick;
}

void Mock_HAL_SetGpioPin(GPIO_TypeDef* port, uint16_t pin, GPIO_PinState state) {
    int pin_index = 0;
    uint16_t pin_mask = pin;
    while (pin_mask > 1) {
        pin_mask >>= 1;
        pin_index++;
    }
    
    if (pin_index < 16) {
        mock_state.gpio_states[pin_index] = state;
    }
}

void Mock_HAL_SetI2CResponse(HAL_StatusTypeDef status, uint8_t* data, uint16_t size) {
    mock_state.i2c_response = status;
    if (data && size > 0 && size <= sizeof(mock_state.i2c_data)) {
        memcpy(mock_state.i2c_data, data, size);
        mock_state.i2c_data_size = size;
    }
}

void Mock_HAL_SetUARTResponse(HAL_StatusTypeDef status) {
    mock_state.uart_response = status;
}

// ============================================================================
// VÉRIFICATIONS DES MOCKS
// ============================================================================

uint32_t Mock_HAL_GetI2CCallCount(void) {
    return mock_state.i2c_call_count;
}

uint32_t Mock_HAL_GetUARTCallCount(void) {
    return mock_state.uart_call_count;
}

uint32_t Mock_HAL_GetGpioWriteCount(void) {
    return mock_state.gpio_write_count;
}

uint32_t Mock_HAL_GetGPIOWriteCallCount(void) {
    return mock_state.gpio_write_count;
}

#endif // UNITY_NATIVE_TESTS
