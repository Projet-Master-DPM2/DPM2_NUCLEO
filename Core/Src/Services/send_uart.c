#include "send_uart.h"

void StartTaskSendUART(void *argument) {
	printf("\r\nSendUART Task started\r\n");
    for (;;) {
        printf("Hello from FreeRTOS!\r\n");
        osDelay(1000);
    }
}