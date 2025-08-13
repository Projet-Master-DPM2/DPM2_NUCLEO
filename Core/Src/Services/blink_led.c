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
            // Démo: lancer une courte livraison sur 2 canaux
            MotorService_StartDelivery(1);
            osDelay(1000);
            MotorService_StartDelivery(2);
            osDelay(1000);
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