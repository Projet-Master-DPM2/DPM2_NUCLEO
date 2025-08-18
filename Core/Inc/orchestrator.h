#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

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

extern osMessageQueueId_t orchestratorEventQueueHandle;

void StartTaskOrchestrator(void *argument);

#endif // ORCHESTRATOR_H

#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

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

extern osMessageQueueId_t orchestratorEventQueueHandle;

void StartTaskOrchestrator(void *argument);

#endif // ORCHESTRATOR_H


