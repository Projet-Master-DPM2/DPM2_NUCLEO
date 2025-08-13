#include "send_uart.h"
void StartTaskSendUART(void *argument) {
    printf("\r\nUART2 Test Task started\r\n");
    // Service de test simple: logs périodiques sur UART2 via printf
    for (;;) {
        printf("[TEST] FreeRTOS alive\r\n");
        osDelay(1000);
    }
}