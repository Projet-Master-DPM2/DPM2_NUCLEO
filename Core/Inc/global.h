#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdint.h>

typedef enum {
    IDLE,
    ORDERING,
    PAYING,
    DELIVERING,
    SETTINGS,
} MachineState;

extern volatile MachineState machine_interaction;
extern volatile char keypad_choice[3];
extern volatile uint8_t client_order;

extern volatile char* lcd_display;

// ------- Logging levels (compile-time) -------
#ifndef LOG_LEVEL
#define LOG_LEVEL 2 // 0=NONE,1=ERROR,2=WARN,3=INFO,4=DEBUG
#endif

// Logging sécurisé avec masquage des données sensibles
#define LOG_SAFE_ENABLED 1
#define LOG_MAX_SENSITIVE_CHARS 3

// Masquage sécurisé pour logging
static inline void log_mask_sensitive(char* dest, const char* src, size_t max_len) {
    if (!src || !dest || max_len < 4) return;
    size_t len = strlen(src);
    if (len <= LOG_MAX_SENSITIVE_CHARS) {
        strncpy(dest, "***", max_len - 1);
    } else {
        snprintf(dest, max_len, "%.*s***", LOG_MAX_SENSITIVE_CHARS, src);
    }
    dest[max_len - 1] = '\0';
}

#define LOGE(...) do { if (LOG_LEVEL>=1) printf(__VA_ARGS__); } while(0)
#define LOGW(...) do { if (LOG_LEVEL>=2) printf(__VA_ARGS__); } while(0)
#define LOGI(...) do { if (LOG_LEVEL>=3) printf(__VA_ARGS__); } while(0)
#define LOGD(...) do { if (LOG_LEVEL>=4) printf(__VA_ARGS__); } while(0)

// Logging sécurisé pour données sensibles (production uniquement)
#if LOG_SAFE_ENABLED
#define LOGS(...) do { if (LOG_LEVEL>=2) printf(__VA_ARGS__); } while(0)
#else
#define LOGS(...) do {} while(0)
#endif

#include "cmsis_os.h"
// Mutex globaux pour protection des ressources partagées
extern osMutexId_t i2c1Mutex;           // Protection bus I2C
extern osMutexId_t globalStateMutex;    // Protection variables d'état globales
extern osMutexId_t keypadChoiceMutex;   // Protection choix keypad

// Fonctions d'accès thread-safe aux variables globales
MachineState GlobalState_Get(void);
void GlobalState_Set(MachineState newState);
bool GlobalState_GetKeypadChoice(char* dest, size_t destSize);
void GlobalState_SetKeypadChoice(const char* choice);
uint8_t GlobalState_GetClientOrder(void);
void GlobalState_SetClientOrder(uint8_t order);

#endif // GLOBAL_H