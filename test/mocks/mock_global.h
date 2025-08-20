#ifndef MOCK_GLOBAL_H
#define MOCK_GLOBAL_H

// Mock pour global.h sans dépendances STM32/FreeRTOS
#ifdef UNITY_NATIVE_TESTS

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

// Mock des types FreeRTOS (éviter duplication avec mock_freertos.h)
#ifndef MOCK_FREERTOS_H
typedef void* osMutexId_t;
typedef void* osMessageQueueId_t;
#endif

// États de la machine
typedef enum {
    IDLE = 0,
    ORDERING = 1,
    PAYING = 2,
    DELIVERING = 3,
    SETTINGS = 4
} MachineState;

// Structure LCD Message
typedef struct {
    char line1[17];
    char line2[17];
} LcdMessage;

// Types d'événements orchestrateur
typedef enum {
    ORCH_EVT_KEYPAD,
    ORCH_EVT_PAYMENT_OK,
    ORCH_EVT_PAYMENT_CANCEL,
    ORCH_EVT_ERROR_MOTOR,
    ORCH_EVT_DELIVERY_DONE,
    ORCH_EVT_STOCK_LOW,
    ORCH_EVT_NO_NET
} OrchestratorEventType;

typedef struct {
    OrchestratorEventType type;
    union {
        char key;     // for ORCH_EVT_KEYPAD
        uint8_t code; // generic small payload
        struct {
            uint8_t sensorId;
            uint8_t mm;
        } stock;      // for ORCH_EVT_STOCK_LOW
    } data;
} OrchestratorEvent;

// Macros de logging mockées
#define LOGE(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#define LOGW(fmt, ...) printf("[WARN] " fmt "\n", ##__VA_ARGS__)
#define LOGI(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOGD(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

// Variables globales
extern volatile MachineState machine_interaction;
extern volatile char keypad_choice[3];
extern volatile uint8_t client_order;

// Mutex globaux
extern osMutexId_t globalStateMutex;
extern osMutexId_t keypadChoiceMutex;

// Queue de l'orchestrateur
extern osMessageQueueId_t orchestratorEventQueueHandle;

// Fonctions thread-safe mockées (implémentation simplifiée pour tests)
static inline MachineState GlobalState_Get(void) {
    return machine_interaction;
}

static inline void GlobalState_Set(MachineState state) {
    machine_interaction = state;
}

static inline bool GlobalState_GetKeypadChoice(char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size < 3) {
        return false;
    }
    strncpy(buffer, (const char*)keypad_choice, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    return true;
}

static inline void GlobalState_SetKeypadChoice(const char* choice) {
    if (choice) {
        strncpy((char*)keypad_choice, choice, 2);
        keypad_choice[2] = '\0';
    }
}

static inline uint8_t GlobalState_GetClientOrder(void) {
    return client_order;
}

static inline void GlobalState_SetClientOrder(uint8_t order) {
    client_order = order;
}

// Fonction de masquage des données sensibles (simplifiée)
static inline void log_mask_sensitive(char* buffer, size_t size) {
    if (buffer && size > 0) {
        memset(buffer, '*', size - 1);
        buffer[size - 1] = '\0';
    }
}

#endif // UNITY_NATIVE_TESTS

#endif // MOCK_GLOBAL_H
