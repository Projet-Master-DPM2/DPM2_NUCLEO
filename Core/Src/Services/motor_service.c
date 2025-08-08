#include "motor_service.h"
#include "main.h"
#include "queue.h"

// Taille de la file de commandes
#define MOTOR_QUEUE_LENGTH 10

// File de message
static QueueHandle_t motorQueue;

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

  // Création de la file de commandes
  motorQueue = xQueueCreate(MOTOR_QUEUE_LENGTH, sizeof(MotorCommand));

    if (motorQueue == NULL) {
        printf("Erreur allocation motorQueue\r\n");
        while(1);
    }

    // S'assurer que le signal est au repos (LOW)
    HAL_GPIO_WritePin(MUX_IN1_SIG_GPIO_Port, MUX_IN1_SIG_Pin, GPIO_PIN_RESET);

    MotorCommand cmd;

    for (;;) {
        printf("MotorService task en attente...\r\n");
        if (xQueueReceive(motorQueue, &cmd, portMAX_DELAY) == pdTRUE) {
            printf("Commande reçue : moteur %d, cmd = %d\r\n", cmd.motorIndex, cmd.command);
            switch (cmd.command) {
                case MOTOR_CMD_RUN:
                    MotorService_Run(cmd.motorIndex);
                    break;
                case MOTOR_CMD_STOP:
                    MotorService_Stop(cmd.motorIndex);
                    break;
            }
        }
    }
}

// Interface pour envoyer une commande à la tâche
void MotorService_SendCommand(uint8_t motorIndex, MotorCommandType command) {
    printf("MotorService_SendCommand: motorIndex = %d, command = %d\r\n", motorIndex, command);
    if (motorQueue == NULL) {
        // File non prête: ignorer ou bufferiser selon besoin
        return;
    }
    MotorCommand cmd = {
        .motorIndex = motorIndex,
        .command = command
    };
    xQueueSend(motorQueue, &cmd, portMAX_DELAY);
}

// Mappe un code commande (11,12,13,21,22,23) vers un canal 0..6
uint8_t MotorService_OrderToChannel(uint8_t orderCode) {
    switch (orderCode) {
        case 11: return 0;
        case 12: return 1;
        case 13: return 2;
        case 21: return 3;
        case 22: return 4;
        case 23: return 6;
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
