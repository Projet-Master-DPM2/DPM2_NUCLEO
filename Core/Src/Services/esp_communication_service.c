#include "esp_communication_service.h"
#include "orchestrator_service.h"
#include <string.h>

static uint8_t rxByte1;
static char lineBuf1[128];
static size_t lineLen1 = 0;


static void process_line_uart1(const char* line) {
    if (!line) return;
    EspMessageType type = EspComm_ClassifyMessage(line);
    switch (type) {
        case ESP_MSG_NFC_UID: {
            OrchestratorEvent evt = { .type = ORCH_EVT_PAYMENT_OK };
            osMessageQueuePut(orchestratorEventQueueHandle, &evt, 0, 0);
            break;
        }
        case ESP_MSG_NFC_ERR: {
            OrchestratorEvent evt = { .type = ORCH_EVT_PAYMENT_CANCEL };
            osMessageQueuePut(orchestratorEventQueueHandle, &evt, 0, 0);
            break;
        }
        case ESP_MSG_NAK_PAYING_NO_NET: {
            OrchestratorEvent evt = { .type = ORCH_EVT_NO_NET };
            osMessageQueuePut(orchestratorEventQueueHandle, &evt, 0, 0);
            break;
        }
        case ESP_MSG_NAK_PAYMENT_DENIED: {
            OrchestratorEvent evt = { .type = ORCH_EVT_PAYMENT_CANCEL };
            osMessageQueuePut(orchestratorEventQueueHandle, &evt, 0, 0);
            break;
        }
        case ESP_MSG_UNKNOWN:
        default:
            break;
    }
}

EspMessageType EspComm_ClassifyMessage(const char* line) {
    if (!line) return ESP_MSG_UNKNOWN;
    if (strncmp(line, "NFC_UID:", 8) == 0) return ESP_MSG_NFC_UID;
    if (strncmp(line, "NFC_ERR:", 8) == 0) return ESP_MSG_NFC_ERR;
    if (strncmp(line, "NAK:STATE:PAYING:NO_NET", 23) == 0) return ESP_MSG_NAK_PAYING_NO_NET;
    if (strncmp(line, "NAK:PAYMENT:DENIED", 19) == 0) return ESP_MSG_NAK_PAYMENT_DENIED;

    return ESP_MSG_UNKNOWN;
}

void EspComm_SendLine(const char* line) {
    if (!line) return;
    HAL_UART_Transmit(&huart1, (uint8_t*)line, (uint16_t)strlen(line), 50);
    const char crlf[2] = {'\r','\n'};
    HAL_UART_Transmit(&huart1, (uint8_t*)crlf, 2, 50);
}

void StartTaskEspCommunication(void *argument) {
    printf("\r\nESP Communication Task started (UART1)\r\n");
    HAL_UART_Receive_IT(&huart1, &rxByte1, 1);
    for (;;) {
        osDelay(10);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        char c = (char)rxByte1;
        if (c == '\r' || c == '\n') {
            if (lineLen1 > 0) {
                lineBuf1[lineLen1] = '\0';
                process_line_uart1(lineBuf1);
                lineLen1 = 0;
            }
        } else {
            if (lineLen1 < sizeof(lineBuf1) - 1) {
                lineBuf1[lineLen1++] = c;
            } else {
                lineLen1 = 0;
            }
        }
        HAL_UART_Receive_IT(&huart1, &rxByte1, 1);
    }
}
