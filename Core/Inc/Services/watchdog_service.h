#ifndef WATCHDOG_SERVICE_H
#define WATCHDOG_SERVICE_H

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include <stdint.h>
#include <stdbool.h>

// Configuration du watchdog
#define WATCHDOG_TIMEOUT_MS         3000    // 3 secondes timeout
#define WATCHDOG_REFRESH_INTERVAL   1500    // Rafraîchir toutes les 1.5s
#define WATCHDOG_MAX_TASKS          8       // Nombre max de tâches surveillées

// Types de tâches critiques
typedef enum {
    TASK_ORCHESTRATOR = 0,
    TASK_KEYPAD,
    TASK_LCD,
    TASK_ESP_COMM,
    TASK_MOTOR,
    TASK_SENSOR_STOCK,
    TASK_COUNT  // Nombre total de tâches
} WatchdogTaskId_t;

// Structure de surveillance des tâches
typedef struct {
    const char* taskName;
    osThreadId_t* taskHandle;
    uint32_t maxIntervalMs;     // Intervalle max sans heartbeat
    uint32_t lastHeartbeat;     // Dernier timestamp de heartbeat
    bool isEnabled;             // Surveillance activée
    bool isAlive;               // État de la tâche
    uint32_t missedHeartbeats;  // Compteur d'échecs
} TaskHealth_t;

// Statistiques watchdog
typedef struct {
    uint32_t totalResets;           // Nombre total de resets watchdog
    uint32_t lastResetTimestamp;    // Timestamp du dernier reset
    uint32_t totalRefresh;          // Nombre de rafraîchissements
    uint32_t missedRefresh;         // Rafraîchissements manqués
    uint32_t systemUptimeMs;        // Temps de fonctionnement
} WatchdogStats_t;

// API du service watchdog
void Watchdog_Init(void);
void Watchdog_Start(void);
void Watchdog_Stop(void);
void Watchdog_Refresh(void);

// Gestion des tâches surveillées
bool Watchdog_RegisterTask(WatchdogTaskId_t taskId, osThreadId_t* handle, uint32_t maxIntervalMs);
void Watchdog_TaskHeartbeat(WatchdogTaskId_t taskId);
void Watchdog_EnableTask(WatchdogTaskId_t taskId, bool enable);

// Surveillance et diagnostic
bool Watchdog_CheckSystemHealth(void);
bool Watchdog_IsTaskAlive(WatchdogTaskId_t taskId);
WatchdogStats_t Watchdog_GetStats(void);
void Watchdog_PrintStatus(void);

// Gestion des resets
bool Watchdog_WasResetCause(void);
void Watchdog_ClearResetFlag(void);
void Watchdog_HandleReset(void);

// Tâche FreeRTOS
void StartTaskWatchdog(void *argument);

#endif // WATCHDOG_SERVICE_H
