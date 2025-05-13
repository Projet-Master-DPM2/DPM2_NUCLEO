#ifndef SEND_UART_H
#define SEND_UART_H

#include "stm32f4xx_hal.h"
#include "stdio.h"
#include "cmsis_os.h"

extern UART_HandleTypeDef huart2;

void StartTaskSendUART(void *argument);

#endif // SEND_UART_H