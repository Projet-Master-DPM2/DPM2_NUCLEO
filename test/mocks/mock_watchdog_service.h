#ifndef MOCK_WATCHDOG_SERVICE_H
#define MOCK_WATCHDOG_SERVICE_H

#ifdef UNITY_NATIVE_TESTS

#include <stdint.h>
#include <stdbool.h>
#include "mock_freertos.h"

// Types pour le watchdog
typedef enum {
    TASK_ORCHESTRATOR = 0,
    TASK_KEYPAD = 1,
    TASK_LCD = 2,
    TASK_ESP_COMM = 3,
    TASK_COUNT = 4
} WatchdogTaskID;

typedef struct {
    uint32_t lastHeartbeat;
    uint32_t timeoutMs;
    bool enabled;
    osThreadId_t* taskHandle;
} TaskHealth_t;

typedef struct {
    uint32_t totalRefresh;
    uint32_t totalTimeouts;
    uint32_t systemResets;
    uint32_t lastResetTime;
} WatchdogStats_t;

// Fonctions mockées du watchdog
void Watchdog_Init(void);
void Watchdog_Start(void);
void Watchdog_Stop(void);
void Watchdog_Refresh(void);
void Watchdog_TaskHeartbeat(WatchdogTaskID taskId);
bool Watchdog_RegisterTask(WatchdogTaskID taskId, osThreadId_t* taskHandle, uint32_t timeoutMs);
bool Watchdog_CheckSystemHealth(void);
bool Watchdog_WasResetCause(void);
void Watchdog_ClearResetFlag(void);
WatchdogStats_t Watchdog_GetStats(void);
void Watchdog_PrintStatus(void);
bool Watchdog_IsTaskAlive(WatchdogTaskID taskId);
void Watchdog_EnableTask(WatchdogTaskID taskId, bool enabled);

// Fonctions spécifiques aux tests
void StartTaskWatchdog(void* argument);

// Fonctions de contrôle pour tests
void Mock_Watchdog_SetResetFlag(bool flag);
void Mock_Watchdog_Reset(void);

#endif // UNITY_NATIVE_TESTS

#endif // MOCK_WATCHDOG_SERVICE_H
