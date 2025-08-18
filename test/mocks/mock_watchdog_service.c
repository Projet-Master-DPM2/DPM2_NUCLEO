#include "mock_watchdog_service.h"

#ifdef UNITY_NATIVE_TESTS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mock_hal.h"

// État du mock watchdog
static struct {
    bool initialized;
    bool started;
    TaskHealth_t tasks[TASK_COUNT];
    WatchdogStats_t stats;
    bool reset_flag;
} mock_watchdog = {0};

// ============================================================================
// FONCTIONS WATCHDOG MOCKÉES
// ============================================================================

void Watchdog_Init(void) {
    memset(&mock_watchdog, 0, sizeof(mock_watchdog));
    mock_watchdog.initialized = true;
}

void Watchdog_Start(void) {
    if (mock_watchdog.initialized) {
        mock_watchdog.started = true;
    }
}

void Watchdog_Stop(void) {
    mock_watchdog.started = false;
}

void Watchdog_Refresh(void) {
    if (mock_watchdog.started && Watchdog_CheckSystemHealth()) {
        mock_watchdog.stats.totalRefresh++;
    }
}

void Watchdog_TaskHeartbeat(WatchdogTaskID taskId) {
    if (taskId < TASK_COUNT && mock_watchdog.tasks[taskId].enabled) {
        mock_watchdog.tasks[taskId].lastHeartbeat = HAL_GetTick();
    }
}

bool Watchdog_RegisterTask(WatchdogTaskID taskId, osThreadId_t* taskHandle, uint32_t timeoutMs) {
    if (taskId >= TASK_COUNT || !taskHandle) {
        return false;
    }
    
    mock_watchdog.tasks[taskId].taskHandle = taskHandle;
    mock_watchdog.tasks[taskId].timeoutMs = timeoutMs;
    mock_watchdog.tasks[taskId].enabled = true;
    mock_watchdog.tasks[taskId].lastHeartbeat = HAL_GetTick();
    
    return true;
}

bool Watchdog_CheckSystemHealth(void) {
    // Vérifier le scheduler FreeRTOS
    osKernelState_t state = osKernelGetState();
    if (state != osKernelRunning) {
        return false;
    }
    
    // Vérifier chaque tâche enregistrée
    uint32_t currentTime = HAL_GetTick();
    
    for (int i = 0; i < TASK_COUNT; i++) {
        if (mock_watchdog.tasks[i].enabled && mock_watchdog.tasks[i].taskHandle) {
            uint32_t elapsed = currentTime - mock_watchdog.tasks[i].lastHeartbeat;
            if (elapsed > mock_watchdog.tasks[i].timeoutMs) {
                mock_watchdog.stats.totalTimeouts++;
                return false;
            }
        }
    }
    
    return true;
}

bool Watchdog_WasResetCause(void) {
    return mock_watchdog.reset_flag;
}

void Watchdog_ClearResetFlag(void) {
    mock_watchdog.reset_flag = false;
}

WatchdogStats_t Watchdog_GetStats(void) {
    return mock_watchdog.stats;
}

void Watchdog_PrintStatus(void) {
    printf("Watchdog Status:\n");
    printf("  Initialized: %s\n", mock_watchdog.initialized ? "Yes" : "No");
    printf("  Started: %s\n", mock_watchdog.started ? "Yes" : "No");
    printf("  Total Refresh: %u\n", mock_watchdog.stats.totalRefresh);
    printf("  Total Timeouts: %u\n", mock_watchdog.stats.totalTimeouts);
}

bool Watchdog_IsTaskAlive(WatchdogTaskID taskId) {
    if (taskId >= TASK_COUNT || !mock_watchdog.tasks[taskId].enabled) {
        return false;
    }
    
    uint32_t currentTime = HAL_GetTick();
    uint32_t elapsed = currentTime - mock_watchdog.tasks[taskId].lastHeartbeat;
    
    return elapsed <= mock_watchdog.tasks[taskId].timeoutMs;
}

void Watchdog_EnableTask(WatchdogTaskID taskId, bool enabled) {
    if (taskId < TASK_COUNT) {
        mock_watchdog.tasks[taskId].enabled = enabled;
    }
}

void StartTaskWatchdog(void* argument) {
    // Mock de la tâche watchdog - ne fait rien
    (void)argument;
}

// ============================================================================
// FONCTIONS DE CONTRÔLE POUR TESTS
// ============================================================================

void Mock_Watchdog_SetResetFlag(bool flag) {
    mock_watchdog.reset_flag = flag;
}

void Mock_Watchdog_Reset(void) {
    memset(&mock_watchdog, 0, sizeof(mock_watchdog));
}

#endif // UNITY_NATIVE_TESTS
