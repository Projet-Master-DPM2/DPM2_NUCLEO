#ifndef SUPERVISION_SERVICE_H
#define SUPERVISION_SERVICE_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Types d'erreurs de supervision
typedef enum {
  SUPERVISION_ERROR_WATCHDOG_TIMEOUT = 0,
  SUPERVISION_ERROR_WATCHDOG_RESET = 1,
  SUPERVISION_ERROR_TASK_HANG = 2,
  SUPERVISION_ERROR_MEMORY_LOW = 3,
  SUPERVISION_ERROR_CRITICAL_SERVICE_FAILURE = 4,
  SUPERVISION_ERROR_HARDWARE_FAULT = 5,
  SUPERVISION_ERROR_SYSTEM_CRASH = 6,
  SUPERVISION_ERROR_MOTOR_FAILURE = 7,
  SUPERVISION_ERROR_SENSOR_FAILURE = 8
} SupervisionErrorType;

// Structure pour les événements de supervision
typedef struct {
  char error_id[64];
  char machine_id[64];
  SupervisionErrorType error_type;
  char message[256];
  uint32_t timestamp;
} SupervisionEvent;

// API du service de supervision
void SupervisionService_Init(void);
void SupervisionService_SendErrorNotification(SupervisionErrorType error_type, const char* message);
void SupervisionService_SendErrorNotificationEvent(const SupervisionEvent* event);
void SupervisionService_GenerateErrorId(char* error_id, size_t max_len);
void SupervisionService_GetMachineId(char* machine_id, size_t max_len);
void SupervisionService_SetMachineId(const char* machine_id);
const char* SupervisionService_ErrorTypeToString(SupervisionErrorType error_type);
bool SupervisionService_ShouldSendNotification(void);

// Configuration
#define SUPERVISION_NOTIFICATION_COOLDOWN_MS 30000  // 30 secondes
#define SUPERVISION_MAX_MESSAGE_LENGTH 256
#define SUPERVISION_MAX_ERROR_ID_LENGTH 64
#define SUPERVISION_MAX_MACHINE_ID_LENGTH 64

#endif // SUPERVISION_SERVICE_H
