#ifndef MOTOR_SERVICE_H
#define MOTOR_SERVICE_H

#include "stm32f4xx_hal.h"
#include "stdio.h"
#include "cmsis_os.h"

// Plus de queue: notifications directes

void StartTaskMotorService(void *argument);
void MotorService_StartDelivery(uint8_t channel);
uint8_t MotorService_OrderToChannel(uint8_t orderCode);
void MotorService_TestSweep(uint8_t firstChannel, uint8_t lastChannel, uint16_t onTimeMs);

#endif
