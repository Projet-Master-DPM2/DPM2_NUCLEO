#include "keypad_service.h"
#include "global.h"
#include "orchestrator_service.h"
#include "string.h"

// Ports et broches pour les lignes et colonnes
static GPIO_TypeDef* rowPorts[KEYPAD_ROWS] = {GPIOA, GPIOA, GPIOB, GPIOC};
static uint16_t rowPins[KEYPAD_ROWS] = {GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_7};

static GPIO_TypeDef* colPorts[KEYPAD_COLS] = {GPIOA, GPIOB, GPIOB};
static uint16_t colPins[KEYPAD_COLS] = {GPIO_PIN_10, GPIO_PIN_4, GPIO_PIN_5};

static char keymap[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
};

// ---------- Queue d'event unifiée ----------
extern osMessageQueueId_t orchestratorEventQueueHandle;

void Keypad_Init(void) {
    for (int i = 0; i < KEYPAD_ROWS; i++) {
        HAL_GPIO_WritePin(rowPorts[i], rowPins[i], GPIO_PIN_SET);
    }
}

static void setAllRowsHigh(void) {
    for (int i = 0; i < KEYPAD_ROWS; i++) {
        HAL_GPIO_WritePin(rowPorts[i], rowPins[i], GPIO_PIN_SET);
    }
}

char Keypad_Scan(void) {
    for (int row = 0; row < KEYPAD_ROWS; row++) {
        setAllRowsHigh();
        HAL_GPIO_WritePin(rowPorts[row], rowPins[row], GPIO_PIN_RESET);
        HAL_Delay(1);
        for (int col = 0; col < KEYPAD_COLS; col++) {
            if (HAL_GPIO_ReadPin(colPorts[col], colPins[col]) == GPIO_PIN_RESET) {
                char detected = keymap[row][col];
                // Anti-rebond simple non bloquant
                osDelay(20);
                // Attendre que la touche soit relâchée sans boucle serrée
                while (HAL_GPIO_ReadPin(colPorts[col], colPins[col]) == GPIO_PIN_RESET) {
                    osDelay(5);
                }
                return detected;
            }
        }
    }
    return 0;
}

void StartTaskKeypad(void *argument) {
    printf("\r\nKeypad Task started\r\n");
    Keypad_Init();

    for (;;) {
        char key = Keypad_Scan();
        if (key) {
            // Envoi vers l'orchestrateur (voie unifiée)
            OrchestratorEvent oevt = { .type = ORCH_EVT_KEYPAD };
            oevt.data.key = key;
            osMessageQueuePut(orchestratorEventQueueHandle, &oevt, 0, 0);
            osDelay(80); // évite répétitions rapides
        }
    }
}