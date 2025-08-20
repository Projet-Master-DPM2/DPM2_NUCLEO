#include "mock_global.h"

#ifdef UNITY_NATIVE_TESTS

// Définition des variables globales mockées
volatile MachineState machine_interaction = IDLE;
volatile char keypad_choice[3] = "";
volatile uint8_t client_order = 0;

// Mutex globaux
osMutexId_t globalStateMutex = NULL;
osMutexId_t keypadChoiceMutex = NULL;

// Queue de l'orchestrateur (pour motor service)
osMessageQueueId_t orchestratorEventQueueHandle = NULL;

#endif // UNITY_NATIVE_TESTS
