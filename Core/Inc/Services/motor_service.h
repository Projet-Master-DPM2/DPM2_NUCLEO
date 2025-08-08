#ifndef MOTOR_SERVICE_H
#define MOTOR_SERVICE_H

#include "stm32f4xx_hal.h"
#include "stdio.h"
#include "cmsis_os.h"

// Enum pour les commandes possibles
typedef enum {
    MOTOR_CMD_RUN,
    MOTOR_CMD_STOP
} MotorCommandType;

// Structure d'une commande moteur
typedef struct {
    uint8_t motorIndex;
    MotorCommandType command;
} MotorCommand;

void StartTaskMotorService(void *argument);
void MotorService_SendCommand(uint8_t motorIndex, MotorCommandType command);
uint8_t MotorService_OrderToChannel(uint8_t orderCode);
void MotorService_TestSweep(uint8_t firstChannel, uint8_t lastChannel, uint16_t onTimeMs);

#endif
