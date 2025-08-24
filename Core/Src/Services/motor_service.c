#include "motor_service.h"
#include "main.h"
#include "queue.h"
#include "orchestrator.h"

// Notification directe + structure de job
typedef struct {
    uint8_t channel;
} MotorJob;

static TaskHandle_t motorTaskHandleLocal = NULL;
static MotorJob currentJob = {0};

void MotorService_SelectMotor(uint8_t index) {
    HAL_GPIO_WritePin(MUX_S0_GPIO_Port, MUX_S0_Pin, (index >> 0) & 0x01 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MUX_S1_GPIO_Port, MUX_S1_Pin, (index >> 1) & 0x01 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MUX_S2_GPIO_Port, MUX_S2_Pin, (index >> 2) & 0x01 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MUX_S3_GPIO_Port, MUX_S3_Pin, (index >> 3) & 0x01 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    printf("MUX select -> S3:%d S2:%d S1:%d S0:%d (idx=%d)\r\n",
           (index >> 3) & 1, (index >> 2) & 1, (index >> 1) & 1, (index >> 0) & 1, index);
}

static void MotorService_Run(uint8_t channel) {
    printf("MotorService_Run: channel = %d\r\n", channel);
    MotorService_SelectMotor(channel);
    // Temps de stabilisation du mux
    osDelay(20);
    HAL_GPIO_WritePin(MUX_IN1_SIG_GPIO_Port, MUX_IN1_SIG_Pin, GPIO_PIN_SET);
}

static void MotorService_Stop(uint8_t channel) {
    printf("MotorService_Stop: channel = %d\r\n", channel);
    MotorService_SelectMotor(channel);
    // Couper le signal
    HAL_GPIO_WritePin(MUX_IN1_SIG_GPIO_Port, MUX_IN1_SIG_Pin, GPIO_PIN_RESET);
}

// Tâche principale FreeRTOS
void StartTaskMotorService(void *argument) {
  printf("MotorService Task started\r\n");

    motorTaskHandleLocal = xTaskGetCurrentTaskHandle();

    // S'assurer que le signal est au repos (LOW)
    HAL_GPIO_WritePin(MUX_IN1_SIG_GPIO_Port, MUX_IN1_SIG_Pin, GPIO_PIN_RESET);

    for (;;) {
        printf("MotorService task en attente...\r\n");
        // Attendre une notification (job disponible)
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        printf("Motor job: ch=%d\r\n", currentJob.channel);
        MotorService_Run(currentJob.channel);
        osDelay(700);
        MotorService_Stop(currentJob.channel);
        // Notifier orchestrateur fin de livraison
        extern osMessageQueueId_t orchestratorEventQueueHandle;
        OrchestratorEvent evt = { .type = ORCH_EVT_DELIVERY_DONE };
        osMessageQueuePut(orchestratorEventQueueHandle, &evt, 0, 0);
    }
}

// API: démarrer une distribution (canal + durée)
void MotorService_StartDelivery(uint8_t channel) {
    if (motorTaskHandleLocal == NULL) return;
    currentJob.channel = channel;
    xTaskNotifyGive(motorTaskHandleLocal);
}

// Mappe un code commande (11,12,13,21,22,23) vers un canal 1..4 (multiplexeur 4 channels)
uint8_t MotorService_OrderToChannel(uint8_t orderCode) {
    switch (orderCode) {
        case 11: return 1;  // Channel 1 du multiplexeur
        case 12: return 2;  // Channel 2 du multiplexeur
        case 13: return 3;  // Channel 3 du multiplexeur
        case 21: return 4;  // Channel 4 du multiplexeur
        case 22: return 1;  // Retour au channel 1 (rotation)
        case 23: return 2;  // Retour au channel 2 (rotation)
        default: return 0xFF; // invalide
    }
}

void MotorService_TestSweep(uint8_t firstChannel, uint8_t lastChannel, uint16_t onTimeMs) {
    if (firstChannel > lastChannel) return;
    for (uint8_t ch = firstChannel; ch <= lastChannel; ++ch) {
        printf("TestSweep: ch=%d\r\n", ch);
        MotorService_Run(ch);
        osDelay(onTimeMs);
        MotorService_Stop(ch);
        osDelay(200);
    }
}
