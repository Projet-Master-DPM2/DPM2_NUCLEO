#ifndef MOCK_MOTOR_SERVICE_H
#define MOCK_MOTOR_SERVICE_H

// Mock pour motor_service.h sans dépendances STM32
#include "mock_hal.h"
#include "mock_freertos.h"
#include "mock_motor_pins.h"
#include <stdio.h>

// Déclarations des fonctions du motor service
void StartTaskMotorService(void *argument);
void MotorService_StartDelivery(uint8_t channel);
uint8_t MotorService_OrderToChannel(uint8_t orderCode);
void MotorService_TestSweep(uint8_t firstChannel, uint8_t lastChannel, uint16_t onTimeMs);

// Variables de test pour capturer l'état
extern uint8_t mock_last_motor_channel;
extern uint8_t mock_motor_running;

// Fonctions de test
void Mock_Motor_Reset(void);

#endif // MOCK_MOTOR_SERVICE_H
