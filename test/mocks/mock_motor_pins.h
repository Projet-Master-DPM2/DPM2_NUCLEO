#ifndef MOCK_MOTOR_PINS_H
#define MOCK_MOTOR_PINS_H

#include "mock_hal.h"

// Définitions des pins pour les tests du motor service
// Ces valeurs correspondent à celles de main.h

// Pins du multiplexeur
#define MUX_S0_Pin          GPIO_PIN_0
#define MUX_S1_Pin          GPIO_PIN_1  
#define MUX_S2_Pin          GPIO_PIN_2
#define MUX_S3_Pin          GPIO_PIN_3

// Pin du signal moteur
#define MUX_IN1_SIG_Pin     GPIO_PIN_0

// GPIO Ports (pointeurs fictifs pour les tests)
extern GPIO_TypeDef* MUX_S0_GPIO_Port;
extern GPIO_TypeDef* MUX_S1_GPIO_Port;
extern GPIO_TypeDef* MUX_S2_GPIO_Port;
extern GPIO_TypeDef* MUX_S3_GPIO_Port;
extern GPIO_TypeDef* MUX_IN1_SIG_GPIO_Port;

// Variables pour capturer l'état des pins dans les tests
extern uint16_t mock_mux_s0_state;
extern uint16_t mock_mux_s1_state;
extern uint16_t mock_mux_s2_state;
extern uint16_t mock_mux_s3_state;
extern uint16_t mock_signal_state;

// Fonction pour réinitialiser l'état des pins
void Mock_Motor_Pins_Reset(void);

#endif // MOCK_MOTOR_PINS_H
