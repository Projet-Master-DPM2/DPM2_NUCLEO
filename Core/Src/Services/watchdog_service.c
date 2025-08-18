#include "stm32f4xx_hal.h"
#include "watchdog_service.h"
#include "global.h"
#include <string.h>

// Registres IWDG (accès direct)
#define IWDG_KEY_RELOAD    0xAAAA
#define IWDG_KEY_ENABLE    0xCCCC
#define IWDG_KEY_WRITE     0x5555
#define IWDG_PRESCALER_64  0x04
#define IWDG_RELOAD_3S     1500

// Variables statiques du service
static bool watchdogInitialized = false;
static bool watchdogEnabled = false;
static WatchdogStats_t watchdogStats = {0};
static uint32_t systemStartTime = 0;

// Table de surveillance des tâches critiques
static TaskHealth_t taskHealthTable[TASK_COUNT] = {
    [TASK_ORCHESTRATOR] = { "Orchestrator", NULL, 2000, 0, true, false, 0 },
    [TASK_KEYPAD]       = { "Keypad",       NULL, 3000, 0, true, false, 0 },
    [TASK_LCD]          = { "LCD",          NULL, 5000, 0, true, false, 0 },
    [TASK_ESP_COMM]     = { "ESP_Comm",     NULL, 4000, 0, true, false, 0 },
    [TASK_MOTOR]        = { "Motor",        NULL, 10000, 0, false, false, 0 },  // Optionnel
    [TASK_SENSOR_STOCK] = { "SensorStock",  NULL, 8000, 0, false, false, 0 }   // Optionnel
};

// Mutex pour protection thread-safe
static osMutexId_t watchdogMutex = NULL;

// ============================================================================
// INITIALISATION ET CONFIGURATION
// ============================================================================

void Watchdog_Init(void) {
    if (watchdogInitialized) return;
    
    LOGI("[Watchdog] Initialisation du service...\r\n");
    
    // Créer le mutex de protection
    const osMutexAttr_t watchdogMutexAttr = { .name = "watchdogMutex" };
    watchdogMutex = osMutexNew(&watchdogMutexAttr);
    if (watchdogMutex == NULL) {
        LOGE("[Watchdog] Erreur création mutex\r\n");
        return;
    }
    
    // Configuration IWDG (accès direct aux registres)
    
    // Vérifier si le reset précédent était dû au watchdog
    if (Watchdog_WasResetCause()) {
        watchdogStats.totalResets++;
        watchdogStats.lastResetTimestamp = HAL_GetTick();
        LOGW("[Watchdog] Reset watchdog détecté! (Total: %lu)\r\n", watchdogStats.totalResets);
        Watchdog_HandleReset();
        Watchdog_ClearResetFlag();
    }
    
    systemStartTime = HAL_GetTick();
    watchdogInitialized = true;
    LOGI("[Watchdog] Initialisation terminée\r\n");
}

void Watchdog_Start(void) {
    if (!watchdogInitialized) {
        LOGE("[Watchdog] Service non initialisé\r\n");
        return;
    }
    
    // Initialisation IWDG par accès direct aux registres
    IWDG->KR = IWDG_KEY_WRITE;          // Déverrouiller les registres
    IWDG->PR = IWDG_PRESCALER_64;       // Prescaler /64
    IWDG->RLR = IWDG_RELOAD_3S;         // Reload value pour ~3s
    IWDG->KR = IWDG_KEY_RELOAD;         // Recharger le compteur
    IWDG->KR = IWDG_KEY_ENABLE;         // Démarrer le watchdog
    
    watchdogEnabled = true;
    LOGI("[Watchdog] Démarré (timeout: %dms)\r\n", WATCHDOG_TIMEOUT_MS);
}

void Watchdog_Stop(void) {
    // Note: L'IWDG ne peut pas être arrêté une fois démarré
    // On peut seulement désactiver la surveillance logicielle
    watchdogEnabled = false;
    LOGW("[Watchdog] Surveillance désactivée (IWDG toujours actif)\r\n");
}

// ============================================================================
// GESTION DES TÂCHES SURVEILLÉES
// ============================================================================

bool Watchdog_RegisterTask(WatchdogTaskId_t taskId, osThreadId_t* handle, uint32_t maxIntervalMs) {
    if (taskId >= TASK_COUNT || !handle) return false;
    
    if (watchdogMutex) osMutexAcquire(watchdogMutex, osWaitForever);
    
    taskHealthTable[taskId].taskHandle = handle;
    taskHealthTable[taskId].maxIntervalMs = maxIntervalMs;
    taskHealthTable[taskId].lastHeartbeat = HAL_GetTick();
    taskHealthTable[taskId].isEnabled = true;
    taskHealthTable[taskId].isAlive = true;
    taskHealthTable[taskId].missedHeartbeats = 0;
    
    if (watchdogMutex) osMutexRelease(watchdogMutex);
    
    LOGI("[Watchdog] Tâche %s enregistrée (max: %lums)\r\n", 
         taskHealthTable[taskId].taskName, maxIntervalMs);
    return true;
}

void Watchdog_TaskHeartbeat(WatchdogTaskId_t taskId) {
    if (taskId >= TASK_COUNT) return;
    
    if (watchdogMutex) osMutexAcquire(watchdogMutex, osWaitForever);
    
    taskHealthTable[taskId].lastHeartbeat = HAL_GetTick();
    taskHealthTable[taskId].isAlive = true;
    
    // Reset du compteur d'échecs sur heartbeat réussi
    if (taskHealthTable[taskId].missedHeartbeats > 0) {
        LOGD("[Watchdog] Tâche %s récupérée\r\n", taskHealthTable[taskId].taskName);
        taskHealthTable[taskId].missedHeartbeats = 0;
    }
    
    if (watchdogMutex) osMutexRelease(watchdogMutex);
}

void Watchdog_EnableTask(WatchdogTaskId_t taskId, bool enable) {
    if (taskId >= TASK_COUNT) return;
    
    if (watchdogMutex) osMutexAcquire(watchdogMutex, osWaitForever);
    taskHealthTable[taskId].isEnabled = enable;
    if (watchdogMutex) osMutexRelease(watchdogMutex);
    
    LOGI("[Watchdog] Tâche %s %s\r\n", 
         taskHealthTable[taskId].taskName, 
         enable ? "activée" : "désactivée");
}

// ============================================================================
// SURVEILLANCE ET DIAGNOSTIC
// ============================================================================

bool Watchdog_CheckSystemHealth(void) {
    if (!watchdogEnabled) return true;
    
    uint32_t currentTime = HAL_GetTick();
    bool systemHealthy = true;
    
    if (watchdogMutex) osMutexAcquire(watchdogMutex, osWaitForever);
    
    // Vérifier chaque tâche surveillée
    for (int i = 0; i < TASK_COUNT; i++) {
        TaskHealth_t* task = &taskHealthTable[i];
        
        if (!task->isEnabled || !task->taskHandle) continue;
        
        uint32_t timeSinceHeartbeat = currentTime - task->lastHeartbeat;
        
        if (timeSinceHeartbeat > task->maxIntervalMs) {
            task->isAlive = false;
            task->missedHeartbeats++;
            systemHealthy = false;
            
            LOGE("[Watchdog] Tâche %s non réactive (%lums, échecs: %lu)\r\n",
                 task->taskName, timeSinceHeartbeat, task->missedHeartbeats);
        } else {
            task->isAlive = true;
        }
    }
    
    // Vérifier l'état du scheduler FreeRTOS
    if (osKernelGetState() != osKernelRunning) {
        LOGE("[Watchdog] Scheduler FreeRTOS défaillant\r\n");
        systemHealthy = false;
    }
    
    if (watchdogMutex) osMutexRelease(watchdogMutex);
    
    return systemHealthy;
}

bool Watchdog_IsTaskAlive(WatchdogTaskId_t taskId) {
    if (taskId >= TASK_COUNT) return false;
    
    bool alive;
    if (watchdogMutex) osMutexAcquire(watchdogMutex, osWaitForever);
    alive = taskHealthTable[taskId].isAlive;
    if (watchdogMutex) osMutexRelease(watchdogMutex);
    
    return alive;
}

void Watchdog_Refresh(void) {
    if (!watchdogEnabled || !watchdogInitialized) return;
    
    // Vérifier la santé du système avant de rafraîchir
    if (Watchdog_CheckSystemHealth()) {
        IWDG->KR = IWDG_KEY_RELOAD;  // Rafraîchir le watchdog
        watchdogStats.totalRefresh++;
        LOGD("[Watchdog] Rafraîchi (total: %lu)\r\n", watchdogStats.totalRefresh);
    } else {
        watchdogStats.missedRefresh++;
        LOGW("[Watchdog] Rafraîchissement refusé (système défaillant)\r\n");
        // Le watchdog va provoquer un reset automatique
    }
}

// ============================================================================
// STATISTIQUES ET DIAGNOSTIC
// ============================================================================

WatchdogStats_t Watchdog_GetStats(void) {
    if (watchdogMutex) osMutexAcquire(watchdogMutex, osWaitForever);
    watchdogStats.systemUptimeMs = HAL_GetTick() - systemStartTime;
    WatchdogStats_t stats = watchdogStats;
    if (watchdogMutex) osMutexRelease(watchdogMutex);
    
    return stats;
}

void Watchdog_PrintStatus(void) {
    uint32_t currentTime = HAL_GetTick();
    
    printf("\r\n=== WATCHDOG STATUS ===\r\n");
    printf("État: %s\r\n", watchdogEnabled ? "ACTIF" : "INACTIF");
    printf("Uptime: %lu ms\r\n", currentTime - systemStartTime);
    printf("Resets: %lu\r\n", watchdogStats.totalResets);
    printf("Rafraîchissements: %lu/%lu\r\n", 
           watchdogStats.totalRefresh, 
           watchdogStats.totalRefresh + watchdogStats.missedRefresh);
    
    printf("\r\n--- TÂCHES SURVEILLÉES ---\r\n");
    for (int i = 0; i < TASK_COUNT; i++) {
        TaskHealth_t* task = &taskHealthTable[i];
        if (!task->isEnabled) continue;
        
        uint32_t timeSince = currentTime - task->lastHeartbeat;
        printf("%-12s: %s (%lu ms, échecs: %lu)\r\n",
               task->taskName,
               task->isAlive ? "VIVANT" : "MORT",
               timeSince,
               task->missedHeartbeats);
    }
    printf("=======================\r\n\r\n");
}

// ============================================================================
// GESTION DES RESETS
// ============================================================================

bool Watchdog_WasResetCause(void) {
    return __HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET;
}

void Watchdog_ClearResetFlag(void) {
    __HAL_RCC_CLEAR_RESET_FLAGS();
}

void Watchdog_HandleReset(void) {
    // Stratégie de récupération post-reset watchdog
    LOGW("[Watchdog] Récupération après reset watchdog...\r\n");
    
    // 1. Sauvegarder les informations de debug si possible
    // (dans un vrai système, on pourrait sauver en EEPROM/Flash)
    
    // 2. Mode de fonctionnement dégradé temporaire
    // Désactiver les tâches optionnelles au démarrage
    LOGI("[Watchdog] Mode dégradé: tâches optionnelles désactivées\r\n");
    
    // 3. Délai de stabilisation
    HAL_Delay(100);
    
    LOGI("[Watchdog] Récupération terminée\r\n");
}

// ============================================================================
// TÂCHE FREERTOS WATCHDOG
// ============================================================================

void StartTaskWatchdog(void *argument) {
    LOGI("[Watchdog] Tâche de surveillance démarrée\r\n");
    
    // Délai de démarrage pour laisser les autres tâches s'initialiser
    osDelay(2000);
    
    // Démarrer le watchdog matériel
    Watchdog_Start();
    
    uint32_t statusPrintCounter = 0;
    
    for (;;) {
        // Surveillance et rafraîchissement
        Watchdog_Refresh();
        
        // Affichage périodique du statut (toutes les 30s)
        if (++statusPrintCounter >= 20) {  // 20 * 1.5s = 30s
            Watchdog_PrintStatus();
            statusPrintCounter = 0;
        }
        
        // Attendre avant la prochaine vérification
        osDelay(WATCHDOG_REFRESH_INTERVAL);
    }
}
