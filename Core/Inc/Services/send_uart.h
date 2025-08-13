#ifndef SEND_UART_H
#define SEND_UART_H

#include "stm32f4xx_hal.h"
#include "stdio.h"
#include "cmsis_os.h"

extern UART_HandleTypeDef huart2;

// Tâche de gestion UART (réception/parsing ligne)
void StartTaskSendUART(void *argument);

// Envoi d'une ligne (ajoute automatiquement "\r\n")
void Uart_SendLine(const char* line);

#endif // SEND_UART_H