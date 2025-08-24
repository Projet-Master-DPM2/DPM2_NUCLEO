#include "esp_communication_service.h"
#include "orchestrator.h"
#include "watchdog_service.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#define UART_BUFFER_SIZE 128
#define UART_MAX_LINE_LENGTH (UART_BUFFER_SIZE - 1)
#define UART_MAX_INVALID_CHARS 10
#define UART_TIMEOUT_MS 1000

static uint8_t rxByte1;
static char lineBuf1[UART_BUFFER_SIZE];
static size_t lineLen1 = 0;
static uint32_t invalidCharCount = 0;
static uint32_t lastRxTimestamp = 0;

// Variables pour la gestion des commandes de livraison
static bool orderInProgress = false;
static char currentOrderId[32] = {0};
static uint8_t totalItems = 0;
static uint8_t deliveredItems = 0;


// Fonction helper pour parser une commande VEND
static bool parse_vend_command(const char* line, uint8_t* slot_number, uint8_t* quantity, char* product_id) {
    if (!line || !slot_number || !quantity || !product_id) return false;
    
    // Format attendu: "VEND <slot_number> <quantity> <product_id>"
    int parsed_slot, parsed_qty;
    char parsed_product[32];
    
    if (sscanf(line, "VEND %d %d %31s", &parsed_slot, &parsed_qty, parsed_product) == 3) {
        if (parsed_slot >= 1 && parsed_slot <= 4 && parsed_qty >= 1 && parsed_qty <= 10) {
            *slot_number = (uint8_t)parsed_slot;
            *quantity = (uint8_t)parsed_qty;
            strncpy(product_id, parsed_product, 31);
            product_id[31] = '\0';
            return true;
        }
    }
    return false;
}

// Fonction helper pour extraire l'ID de commande
static bool parse_order_start(const char* line, char* order_id) {
    if (!line || !order_id) return false;
    
    // Format attendu: "ORDER_START:<order_id>"
    if (sscanf(line, "ORDER_START:%31s", order_id) == 1) {
        return true;
    }
    return false;
}

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
        case ESP_MSG_ORDER_START: {
            if (parse_order_start(line, currentOrderId)) {
                orderInProgress = true;
                totalItems = 0;
                deliveredItems = 0;
                
                OrchestratorEvent evt = { .type = ORCH_EVT_ORDER_START };
                strncpy(evt.data.order.order_id, currentOrderId, sizeof(evt.data.order.order_id) - 1);
                evt.data.order.order_id[sizeof(evt.data.order.order_id) - 1] = '\0';
                osMessageQueuePut(orchestratorEventQueueHandle, &evt, 0, 0);
                
                // Confirmer la réception de la commande
                EspComm_SendLine("ORDER_ACK");
                printf("[ESP_UART] Order started: %s\r\n", currentOrderId);
            }
            break;
        }
        case ESP_MSG_VEND_COMMAND: {
            if (!orderInProgress) {
                printf("[ESP_UART] VEND command received without active order\r\n");
                EspComm_SendLine("ORDER_NAK:NO_ACTIVE_ORDER");
                break;
            }
            
            uint8_t slot_number, quantity;
            char product_id[32];
            
            if (parse_vend_command(line, &slot_number, &quantity, product_id)) {
                totalItems++;
                
                OrchestratorEvent evt = { .type = ORCH_EVT_VEND_ITEM };
                evt.data.vend.slot_number = slot_number;
                evt.data.vend.quantity = quantity;
                strncpy(evt.data.vend.product_id, product_id, sizeof(evt.data.vend.product_id) - 1);
                evt.data.vend.product_id[sizeof(evt.data.vend.product_id) - 1] = '\0';
                osMessageQueuePut(orchestratorEventQueueHandle, &evt, 0, 0);
                
                printf("[ESP_UART] VEND command: slot=%d, qty=%d, product=%s\r\n", 
                       slot_number, quantity, product_id);
            } else {
                printf("[ESP_UART] Invalid VEND command format: %s\r\n", line);
                EspComm_SendLine("ORDER_NAK:INVALID_VEND_FORMAT");
            }
            break;
        }
        case ESP_MSG_ORDER_END: {
            if (orderInProgress) {
                orderInProgress = false;
                
                OrchestratorEvent evt = { .type = ORCH_EVT_ORDER_COMPLETE };
                osMessageQueuePut(orchestratorEventQueueHandle, &evt, 0, 0);
                
                printf("[ESP_UART] Order completed: %s (total items: %d)\r\n", 
                       currentOrderId, totalItems);
            }
            break;
        }
        case ESP_MSG_QR_TOKEN_ERROR:
        case ESP_MSG_QR_TOKEN_INVALID:
        case ESP_MSG_QR_TOKEN_BUSY:
        case ESP_MSG_QR_TOKEN_NO_NETWORK:
        case ESP_MSG_ORDER_FAILED: {
            // Réinitialiser l'état de commande en cas d'erreur
            if (orderInProgress) {
                orderInProgress = false;
                OrchestratorEvent evt = { .type = ORCH_EVT_ORDER_FAILED };
                osMessageQueuePut(orchestratorEventQueueHandle, &evt, 0, 0);
                printf("[ESP_UART] Order failed/cancelled: %s\r\n", line);
            }
            break;
        }
        case ESP_MSG_UNKNOWN:
        default:
            printf("[ESP_UART] Unknown message: %s\r\n", line);
            break;
    }
}

EspMessageType EspComm_ClassifyMessage(const char* line) {
    if (!line) return ESP_MSG_UNKNOWN;
    if (strncmp(line, "NFC_UID:", 8) == 0) return ESP_MSG_NFC_UID;
    if (strncmp(line, "NFC_ERR:", 8) == 0) return ESP_MSG_NFC_ERR;
    if (strncmp(line, "NAK:STATE:PAYING:NO_NET", 23) == 0) return ESP_MSG_NAK_PAYING_NO_NET;
    if (strncmp(line, "NAK:PAYMENT:DENIED", 19) == 0) return ESP_MSG_NAK_PAYMENT_DENIED;
    if (strncmp(line, "ORDER_START:", 12) == 0) return ESP_MSG_ORDER_START;
    if (strncmp(line, "VEND ", 5) == 0) return ESP_MSG_VEND_COMMAND;
    if (strcmp(line, "ORDER_END") == 0) return ESP_MSG_ORDER_END;
    if (strcmp(line, "QR_TOKEN_ERROR") == 0) return ESP_MSG_QR_TOKEN_ERROR;
    if (strcmp(line, "QR_TOKEN_INVALID") == 0) return ESP_MSG_QR_TOKEN_INVALID;
    if (strcmp(line, "QR_TOKEN_BUSY") == 0) return ESP_MSG_QR_TOKEN_BUSY;
    if (strcmp(line, "QR_TOKEN_NO_NETWORK") == 0) return ESP_MSG_QR_TOKEN_NO_NETWORK;
    if (strcmp(line, "ORDER_FAILED") == 0) return ESP_MSG_ORDER_FAILED;

    return ESP_MSG_UNKNOWN;
}

// Validation sécurisée des caractères UART
static bool EspComm_IsValidChar(char c) {
    // Autoriser seulement caractères imprimables ASCII + CR/LF
    return (c >= 0x20 && c <= 0x7E) || c == '\r' || c == '\n';
}

// Reset sécurisé du buffer avec logging
static void EspComm_ResetBuffer(const char* reason) {
    if (lineLen1 > 0) {
        LOGW("[ESP_UART] Buffer reset: %s (len=%zu)\r\n", reason, lineLen1);
    }
    lineLen1 = 0;
    memset(lineBuf1, 0, sizeof(lineBuf1));
}

void EspComm_SendLine(const char* line) {
    if (!line) return;
    
    size_t len = strlen(line);
    if (len == 0 || len > UART_MAX_LINE_LENGTH) {
        LOGE("[ESP_UART] Invalid send length: %zu\r\n", len);
        return;
    }
    
    // Validation des caractères avant envoi
    for (size_t i = 0; i < len; i++) {
        if (!EspComm_IsValidChar(line[i])) {
            LOGE("[ESP_UART] Invalid char in send: 0x%02X\r\n", (uint8_t)line[i]);
            return;
        }
    }
    
    HAL_UART_Transmit(&huart1, (uint8_t*)line, (uint16_t)len, 100);
    const char crlf[2] = {'\r','\n'};
    HAL_UART_Transmit(&huart1, (uint8_t*)crlf, 2, 50);
}

void StartTaskEspCommunication(void *argument) {
    printf("\r\nESP Communication Task started (UART1)\r\n");
    HAL_UART_Receive_IT(&huart1, &rxByte1, 1);
    
    uint32_t heartbeatCounter = 0;
    
    for (;;) {
        // Heartbeat watchdog toutes les 200 itérations (10ms * 200 = 2s)
        if (++heartbeatCounter >= 200) {
            Watchdog_TaskHeartbeat(TASK_ESP_COMM);
            heartbeatCounter = 0;
        }
        
        osDelay(10);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        uint32_t currentTime = HAL_GetTick();
        char c = (char)rxByte1;
        
        // Vérification timeout entre caractères
        if (lastRxTimestamp != 0 && (currentTime - lastRxTimestamp) > UART_TIMEOUT_MS) {
            EspComm_ResetBuffer("timeout");
            invalidCharCount = 0;
        }
        lastRxTimestamp = currentTime;
        
        // Traitement fin de ligne
        if (c == '\r' || c == '\n') {
            if (lineLen1 > 0) {
                lineBuf1[lineLen1] = '\0';
                process_line_uart1(lineBuf1);
                EspComm_ResetBuffer("processed");
                invalidCharCount = 0;
            }
        } else {
            // Validation caractère
            if (!EspComm_IsValidChar(c)) {
                invalidCharCount++;
                if (invalidCharCount >= UART_MAX_INVALID_CHARS) {
                    LOGE("[ESP_UART] Too many invalid chars, resetting\r\n");
                    EspComm_ResetBuffer("invalid_chars");
                    invalidCharCount = 0;
                }
            } else {
                // Ajout caractère valide au buffer
                if (lineLen1 < UART_MAX_LINE_LENGTH) {
                    lineBuf1[lineLen1++] = c;
                } else {
                    LOGW("[ESP_UART] Line too long, truncating\r\n");
                    EspComm_ResetBuffer("overflow");
                    invalidCharCount = 0;
                }
            }
        }
        
        HAL_UART_Receive_IT(&huart1, &rxByte1, 1);
    }
}
