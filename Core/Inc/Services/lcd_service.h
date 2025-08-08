#ifndef __LCD_SERVICE_H__
#define __LCD_SERVICE_H__

#include "stm32f4xx_hal.h"
#include "stdio.h"
#include "cmsis_os.h"
#include "i2c.h"

extern I2C_HandleTypeDef hi2c1;

typedef struct {
    char line1[17];
    char line2[17];
} LcdMessage;

extern osMessageQueueId_t lcdMessageQueueHandle;
void LCD_SendMessage(const LcdMessage* msg);

void lcd_send_data(uint8_t data);
void lcd_send_string(char *str);
void lcd_scroll_string(char *str, uint8_t row, uint16_t delay_ms);
void StartTaskLCD(void *argument);

#endif // __LCD_SERVICE_H__
