#ifndef ESP_COMMUNICATION_SERVICE_H
#define ESP_COMMUNICATION_SERVICE_H

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

// Handle UART1 défini dans usart.c
extern UART_HandleTypeDef huart1;

typedef enum {
    ESP_MSG_UNKNOWN = 0,
    ESP_MSG_NFC_UID,
    ESP_MSG_NFC_ERR,
    ESP_MSG_NAK_PAYING_NO_NET,
    ESP_MSG_NAK_PAYMENT_DENIED
} EspMessageType;

// Détecte le type de message en fonction de la ligne reçue
EspMessageType EspComm_ClassifyMessage(const char* line);

// Démarre la tâche de communication avec l'ESP (UART1)
void StartTaskEspCommunication(void *argument);

// Envoie une ligne (CRLF ajouté) vers l'ESP via UART1
void EspComm_SendLine(const char* line);

#endif // ESP_COMMUNICATION_SERVICE_H


