#ifndef KEYPAD_H
#define KEYPAD_H

#include "stm32f4xx_hal.h"
#include "stdio.h"
#include "cmsis_os.h"
#include "gpio.h"
#include "lcd_i2c.h"

#define KEYPAD_ROWS 4
#define KEYPAD_COLS 3

extern UART_HandleTypeDef huart2;

void Keypad_Init(void);
char Keypad_Scan(void);
void StartTaskKeypad(void *argument);
//void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

#endif