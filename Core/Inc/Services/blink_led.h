#ifndef BLINK_LED_H
#define BLINK_LED_H

#include "stm32f4xx_hal.h"
#include "stdio.h"
#include "cmsis_os.h"

void StartTaskBlinkLED(void *argument);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

#endif // BLINK_LED_TASK_H