// blink_led_task.c
#include "blink_led.h"
#include "motor_service.h"

volatile uint8_t ledBlinkActive = 0;

void StartTaskBlinkLED(void *argument) {
    printf("\r\nLED Task started\r\n");    
    for (;;) {
        if (ledBlinkActive) {
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            printf("Blinking LED\r\n");
            MotorService_SendCommand(0, MOTOR_CMD_RUN);
            osDelay(500);
            MotorService_SendCommand(0, MOTOR_CMD_STOP);
            osDelay(500);
            MotorService_SendCommand(1, MOTOR_CMD_RUN);
            osDelay(500);
            MotorService_SendCommand(1, MOTOR_CMD_STOP);
            osDelay(500);
        } else {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
            osDelay(10);
        }
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_13) {
        ledBlinkActive = !ledBlinkActive;
        printf("Bouton pressé ! ledBlinkActive = %d\r\n", ledBlinkActive);
    }
}