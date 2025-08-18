#include "global.h"
#include <string.h>

volatile MachineState machine_interaction = IDLE;
volatile char keypad_choice[3] = "";
volatile uint8_t client_order = 0;

volatile char* lcd_display = "Pret";

// Mutex pour protection thread-safe
osMutexId_t globalStateMutex = NULL;
osMutexId_t keypadChoiceMutex = NULL;

// Fonctions d'accès thread-safe aux variables globales
MachineState GlobalState_Get(void) {
    if (globalStateMutex == NULL) return machine_interaction;
    
    osMutexAcquire(globalStateMutex, osWaitForever);
    MachineState state = machine_interaction;
    osMutexRelease(globalStateMutex);
    return state;
}

void GlobalState_Set(MachineState newState) {
    if (globalStateMutex == NULL) {
        machine_interaction = newState;
        return;
    }
    
    osMutexAcquire(globalStateMutex, osWaitForever);
    machine_interaction = newState;
    osMutexRelease(globalStateMutex);
}

bool GlobalState_GetKeypadChoice(char* dest, size_t destSize) {
    if (!dest || destSize < 3 || keypadChoiceMutex == NULL) return false;
    
    osMutexAcquire(keypadChoiceMutex, osWaitForever);
    strncpy(dest, (const char*)keypad_choice, destSize - 1);
    dest[destSize - 1] = '\0';
    osMutexRelease(keypadChoiceMutex);
    return true;
}

void GlobalState_SetKeypadChoice(const char* choice) {
    if (!choice || keypadChoiceMutex == NULL) return;
    
    osMutexAcquire(keypadChoiceMutex, osWaitForever);
    strncpy((char*)keypad_choice, choice, sizeof(keypad_choice) - 1);
    keypad_choice[sizeof(keypad_choice) - 1] = '\0';
    osMutexRelease(keypadChoiceMutex);
}

uint8_t GlobalState_GetClientOrder(void) {
    if (globalStateMutex == NULL) return client_order;
    
    osMutexAcquire(globalStateMutex, osWaitForever);
    uint8_t order = client_order;
    osMutexRelease(globalStateMutex);
    return order;
}

void GlobalState_SetClientOrder(uint8_t order) {
    if (globalStateMutex == NULL) {
        client_order = order;
        return;
    }
    
    osMutexAcquire(globalStateMutex, osWaitForever);
    client_order = order;
    osMutexRelease(globalStateMutex);
}


