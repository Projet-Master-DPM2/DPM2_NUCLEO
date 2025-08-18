#include "keypad_service.h"
#include "global.h"
#include "orchestrator.h"
#include "watchdog_service.h"
#include "string.h"

#define KEYPAD_MAX_INVALID_ATTEMPTS 10
#define KEYPAD_LOCKOUT_TIME_MS 5000
#define KEYPAD_MIN_PRESS_INTERVAL_MS 100

static uint32_t invalidAttempts = 0;
static uint32_t lastKeyPressTime = 0;
static uint32_t lockoutStartTime = 0;

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

// Validation sécurisée avec rate limiting
static bool Keypad_ValidateInput(char key) {
    uint32_t currentTime = HAL_GetTick();
    
    // Vérifier si en période de lockout
    if (lockoutStartTime != 0) {
        if ((currentTime - lockoutStartTime) < KEYPAD_LOCKOUT_TIME_MS) {
            return false; // Encore en lockout
        } else {
            // Fin du lockout, reset des compteurs
            lockoutStartTime = 0;
            invalidAttempts = 0;
            LOGI("[Keypad] Lockout terminé\r\n");
        }
    }
    
    // Rate limiting : éviter les presses trop rapides
    if (lastKeyPressTime != 0 && (currentTime - lastKeyPressTime) < KEYPAD_MIN_PRESS_INTERVAL_MS) {
        return false;
    }
    
    // Validation caractère
    if (!((key >= '0' && key <= '9') || key == '*' || key == '#')) {
        invalidAttempts++;
        LOGW("[Keypad] Caractère invalide: 0x%02X (tentatives: %lu)\r\n", 
             (uint8_t)key, invalidAttempts);
        
        if (invalidAttempts >= KEYPAD_MAX_INVALID_ATTEMPTS) {
            lockoutStartTime = currentTime;
            LOGE("[Keypad] Trop de tentatives invalides, lockout %d ms\r\n", 
                 KEYPAD_LOCKOUT_TIME_MS);
        }
        return false;
    }
    
    // Caractère valide
    lastKeyPressTime = currentTime;
    if (invalidAttempts > 0) {
        invalidAttempts = 0; // Reset sur succès
    }
    return true;
}

void StartTaskKeypad(void *argument) {
    printf("\r\nKeypad Task started\r\n");
    Keypad_Init();

    uint32_t heartbeatCounter = 0;

    for (;;) {
        // Heartbeat watchdog toutes les ~16 itérations (80ms * 16 = ~1.3s)
        if (++heartbeatCounter >= 16) {
            Watchdog_TaskHeartbeat(TASK_KEYPAD);
            heartbeatCounter = 0;
        }

        char key = Keypad_Scan();
        if (key && Keypad_ValidateInput(key)) {
            // Envoi vers l'orchestrateur (voie unifiée)
            OrchestratorEvent oevt = { .type = ORCH_EVT_KEYPAD };
            oevt.data.key = key;
            osMessageQueuePut(orchestratorEventQueueHandle, &oevt, 0, 0);
            LOGD("[Keypad] Touche valide envoyée\r\n");
            osDelay(80); // évite répétitions rapides
        } else {
            osDelay(5); // Délai plus court pour caractères invalides
        }
    }
}