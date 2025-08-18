#ifndef WATCHDOG_CONFIG_H
#define WATCHDOG_CONFIG_H

// ============================================================================
// CONFIGURATION DU WATCHDOG SYSTÈME
// ============================================================================

// Paramètres temporels (millisecondes)
#define WATCHDOG_TIMEOUT_MS         3000    // Timeout matériel IWDG
#define WATCHDOG_REFRESH_INTERVAL   1500    // Intervalle de rafraîchissement
#define WATCHDOG_STARTUP_DELAY      2000    // Délai avant démarrage watchdog

// Seuils de surveillance des tâches (millisecondes)
#define WATCHDOG_ORCHESTRATOR_MAX   2000    // Tâche critique principale
#define WATCHDOG_KEYPAD_MAX         3000    // Interface utilisateur
#define WATCHDOG_LCD_MAX            5000    // Affichage (peut être lent)
#define WATCHDOG_ESP_COMM_MAX       4000    // Communication ESP32
#define WATCHDOG_MOTOR_MAX          10000   // Moteurs (optionnel)
#define WATCHDOG_SENSOR_MAX         8000    // Capteurs (optionnel)

// Configuration du diagnostic
#define WATCHDOG_STATUS_PRINT_INTERVAL  30  // Affichage statut (secondes)
#define WATCHDOG_MAX_MISSED_HEARTBEATS  3   // Seuil d'alerte heartbeat
#define WATCHDOG_ENABLE_VERBOSE_LOGS    1   // Logs détaillés

// Paramètres de récupération
#define WATCHDOG_ENABLE_SAFE_MODE       1   // Mode dégradé après reset
#define WATCHDOG_RESET_HISTORY_SIZE     10  // Historique des resets
#define WATCHDOG_ENABLE_STATISTICS      1   // Collecte de statistiques

// Configuration matérielle IWDG
// Formule: Timeout = (Prescaler × Reload) / LSI_Freq
// LSI ≈ 32kHz sur STM32F411
#define IWDG_PRESCALER_VALUE    IWDG_PRESCALER_64   // /64
#define IWDG_RELOAD_VALUE       1500                // ~3s avec LSI 32kHz

// Priorités des tâches
#define WATCHDOG_TASK_PRIORITY      osPriorityRealtime  // Priorité maximale
#define WATCHDOG_TASK_STACK_SIZE    (512 * 4)          // Stack suffisant

// Options de compilation conditionnelle
#ifdef DEBUG
    #define WATCHDOG_DEBUG_MODE     1
    #define WATCHDOG_VERBOSE_LOGS   1
#else
    #define WATCHDOG_DEBUG_MODE     0
    #define WATCHDOG_VERBOSE_LOGS   0
#endif

// Validation des paramètres
#if WATCHDOG_REFRESH_INTERVAL >= WATCHDOG_TIMEOUT_MS
    #error "WATCHDOG_REFRESH_INTERVAL must be < WATCHDOG_TIMEOUT_MS"
#endif

#if WATCHDOG_ORCHESTRATOR_MAX >= WATCHDOG_TIMEOUT_MS
    #error "Task timeouts must be < WATCHDOG_TIMEOUT_MS"
#endif

#endif // WATCHDOG_CONFIG_H
