#include "mock_motor_pins.h"
#include <stdio.h>

// GPIO Ports fictifs pour les tests
static GPIO_TypeDef mock_gpioc;
static GPIO_TypeDef mock_gpiob;

// Définir les ports comme dans main.h
GPIO_TypeDef* MUX_S0_GPIO_Port = &mock_gpioc;      // GPIOC
GPIO_TypeDef* MUX_S1_GPIO_Port = &mock_gpioc;      // GPIOC
GPIO_TypeDef* MUX_S2_GPIO_Port = &mock_gpioc;      // GPIOC
GPIO_TypeDef* MUX_S3_GPIO_Port = &mock_gpioc;      // GPIOC
GPIO_TypeDef* MUX_IN1_SIG_GPIO_Port = &mock_gpiob; // GPIOB

// Alias pour compatibilité avec mock_hal.h
GPIO_TypeDef* GPIOC = &mock_gpioc;
GPIO_TypeDef* GPIOB = &mock_gpiob;

// Variables pour capturer l'état des pins
uint16_t mock_mux_s0_state = 0;
uint16_t mock_mux_s1_state = 0;
uint16_t mock_mux_s2_state = 0;
uint16_t mock_mux_s3_state = 0;
uint16_t mock_signal_state = 0;

void Mock_Motor_Pins_Reset(void) {
    mock_mux_s0_state = 0;
    mock_mux_s1_state = 0;
    mock_mux_s2_state = 0;
    mock_mux_s3_state = 0;
    mock_signal_state = 0;
}
