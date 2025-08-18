# 🐕 Watchdog Système - Documentation

## Vue d'Ensemble

Le **service watchdog** implémenté dans le projet DPM2_NUCLEO assure la **surveillance continue** du système et la **récupération automatique** en cas de blocage.

## Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   IWDG (HW)     │    │  Watchdog Task  │    │  Tâches App     │
│                 │    │                 │    │                 │
│ • Timeout: 3s   │◄───│ • Surveillance  │◄───│ • Heartbeats    │
│ • Auto-reset    │    │ • Diagnostic    │    │ • Validation    │
│ • Non-stop      │    │ • Rafraîchit    │    │ • États         │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

## Configuration

### Paramètres Principaux

| Paramètre | Valeur | Description |
|-----------|--------|-------------|
| `WATCHDOG_TIMEOUT_MS` | 3000ms | Timeout matériel IWDG |
| `WATCHDOG_REFRESH_INTERVAL` | 1500ms | Intervalle rafraîchissement |
| `WATCHDOG_STARTUP_DELAY` | 2000ms | Délai avant démarrage |

### Tâches Surveillées

| Tâche | Timeout | Criticité | Description |
|-------|---------|-----------|-------------|
| **Orchestrator** | 2000ms | 🔴 Critique | Logique métier principale |
| **Keypad** | 3000ms | 🔴 Critique | Interface utilisateur |
| **LCD** | 5000ms | 🟡 Important | Affichage (I2C lent) |
| **ESP_Comm** | 4000ms | 🟡 Important | Communication réseau |
| **Motor** | 10000ms | 🟢 Optionnel | Moteurs (désactivé) |
| **SensorStock** | 8000ms | 🟢 Optionnel | Capteurs (désactivé) |

## Utilisation

### Initialisation Automatique

Le watchdog s'initialise automatiquement au démarrage :

```c
// Dans freertos.c - MX_FREERTOS_Init()
Watchdog_Init();
watchdogTaskHandle = osThreadNew(StartTaskWatchdog, NULL, &watchdogTask_attributes);

// Enregistrement des tâches critiques
Watchdog_RegisterTask(TASK_ORCHESTRATOR, &orchestratorTaskHandle, 2000);
Watchdog_RegisterTask(TASK_KEYPAD, &keypadTaskHandle, 3000);
// ...
```

### Heartbeats dans les Tâches

Chaque tâche surveillée doit envoyer des **heartbeats** réguliers :

```c
void StartTaskOrchestrator(void *argument) {
    for (;;) {
        // ✅ Heartbeat obligatoire
        Watchdog_TaskHeartbeat(TASK_ORCHESTRATOR);
        
        // Travail normal de la tâche...
        process_events();
        
        osDelay(100);
    }
}
```

### API Disponible

```c
// Gestion du service
void Watchdog_Init(void);
void Watchdog_Start(void);
void Watchdog_Stop(void);

// Enregistrement des tâches
bool Watchdog_RegisterTask(WatchdogTaskId_t taskId, osThreadId_t* handle, uint32_t maxIntervalMs);
void Watchdog_TaskHeartbeat(WatchdogTaskId_t taskId);
void Watchdog_EnableTask(WatchdogTaskId_t taskId, bool enable);

// Surveillance
bool Watchdog_CheckSystemHealth(void);
bool Watchdog_IsTaskAlive(WatchdogTaskId_t taskId);
WatchdogStats_t Watchdog_GetStats(void);
void Watchdog_PrintStatus(void);

// Gestion des resets
bool Watchdog_WasResetCause(void);
void Watchdog_HandleReset(void);
```

## Fonctionnement

### Cycle de Surveillance

```
1. Tâche Watchdog (priorité max) s'exécute toutes les 1.5s
2. Vérifie la santé de chaque tâche surveillée
3. Si toutes les tâches sont vivantes → Rafraîchit IWDG
4. Si une tâche est bloquée → Laisse IWDG expirer
5. IWDG expire (3s) → Reset automatique du système
```

### Détection de Blocage

Une tâche est considérée **bloquée** si :
- Aucun heartbeat reçu depuis `maxIntervalMs`
- Le scheduler FreeRTOS est défaillant
- La tâche est dans un état incohérent

### Récupération Post-Reset

```c
// Au démarrage dans main()
if (Watchdog_WasResetCause()) {
    printf("*** RESET WATCHDOG DÉTECTÉ ***\n");
    // Mode dégradé temporaire
    // Diagnostic rapide
    // Reprise normale
}
```

## Monitoring

### Statut en Temps Réel

Le watchdog affiche périodiquement son statut :

```
=== WATCHDOG STATUS ===
État: ACTIF
Uptime: 125340 ms
Resets: 0
Rafraîchissements: 83/83

--- TÂCHES SURVEILLÉES ---
Orchestrator : VIVANT (245 ms, échecs: 0)
Keypad       : VIVANT (1340 ms, échecs: 0)
LCD          : VIVANT (2100 ms, échecs: 0)
ESP_Comm     : VIVANT (890 ms, échecs: 0)
=======================
```

### Statistiques Collectées

```c
typedef struct {
    uint32_t totalResets;           // Nombre de resets watchdog
    uint32_t lastResetTimestamp;    // Timestamp du dernier reset
    uint32_t totalRefresh;          // Rafraîchissements réussis
    uint32_t missedRefresh;         // Rafraîchissements manqués
    uint32_t systemUptimeMs;        // Temps de fonctionnement
} WatchdogStats_t;
```

## Sécurité

### Protection Contre les Contournements

- **IWDG matériel** : Impossible à désactiver une fois démarré
- **Priorité maximale** : Tâche watchdog non préemptable
- **Validation stricte** : Vérification santé système avant rafraîchissement
- **Mutex thread-safe** : Protection contre les accès concurrents

### Stratégie de Récupération

1. **Reset Watchdog** → Système redémarre proprement
2. **Mode Dégradé** → Tâches optionnelles désactivées
3. **Auto-diagnostic** → Vérification état système
4. **Reprise Normale** → Retour fonctionnement complet

## Dépannage

### Problèmes Courants

**Reset watchdog fréquents :**
- Vérifier les heartbeats dans toutes les tâches
- Augmenter les timeouts si nécessaire
- Vérifier les blocages I2C/UART

**Tâche marquée comme morte :**
- Ajouter/corriger les heartbeats
- Vérifier les boucles infinies
- Optimiser les délais de traitement

**Système ne démarre plus :**
- Reset matériel (bouton)
- Vérifier l'alimentation
- Reprogrammer le firmware

### Logs de Debug

```c
// Activer les logs détaillés
#define LOG_LEVEL 4  // Debug
#define WATCHDOG_VERBOSE_LOGS 1

// Logs typiques
[Watchdog] Initialisation du service...
[Watchdog] Tâche Orchestrator enregistrée (max: 2000ms)
[Watchdog] Démarré (timeout: 3000ms)
[Watchdog] Rafraîchi (total: 42)
[Watchdog] Tâche Keypad non réactive (3245ms, échecs: 1)
```

## Intégration

### Ajout d'une Nouvelle Tâche

```c
// 1. Ajouter l'ID dans watchdog_service.h
typedef enum {
    TASK_ORCHESTRATOR = 0,
    TASK_KEYPAD,
    TASK_LCD,
    TASK_ESP_COMM,
    TASK_MY_NEW_TASK,  // ← Nouvelle tâche
    TASK_COUNT
} WatchdogTaskId_t;

// 2. Enregistrer dans freertos.c
Watchdog_RegisterTask(TASK_MY_NEW_TASK, &myTaskHandle, 5000);

// 3. Ajouter heartbeats dans la tâche
void StartTaskMyNewTask(void *argument) {
    for (;;) {
        Watchdog_TaskHeartbeat(TASK_MY_NEW_TASK);
        // ... travail de la tâche
        osDelay(1000);
    }
}
```

### Configuration Avancée

Modifier `watchdog_config.h` pour personnaliser :
- Timeouts des tâches
- Intervalles de surveillance
- Options de debug
- Paramètres de récupération

## Conclusion

Le **watchdog système** assure une **protection robuste** contre les blocages et garantit la **continuité de service** du distributeur automatique. Il constitue une **couche de sécurité essentielle** pour un système embarqué critique.

---

**Équipe DPM2** - Watchdog Système v1.0
