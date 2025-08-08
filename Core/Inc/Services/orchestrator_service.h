#ifndef ORCHESTRATOR_SERVICE_H
#define ORCHESTRATOR_SERVICE_H

#include "global.h"
#include "keypad_service.h"
#include "lcd_service.h"
#include "motor_service.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>

typedef enum {
    ORCH_EVT_KEYPAD,
    ORCH_EVT_PAYMENT_OK,
    ORCH_EVT_PAYMENT_CANCEL,
    ORCH_EVT_ERROR_MOTOR
} OrchestratorEventType;

typedef struct {
    OrchestratorEventType type;
    union {
        char key;     // for ORCH_EVT_KEYPAD
        uint8_t code; // generic small payload
    } data;
} OrchestratorEvent;

extern osMessageQueueId_t orchestratorEventQueueHandle;

void StartTaskOrchestrator(void *argument);

#endif