#include "supervision_service.h"
#include "global.h"
#include "Services/esp_communication_service.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Variables statiques
static char machine_id[SUPERVISION_MAX_MACHINE_ID_LENGTH] = "";
static bool is_initialized = false;
static uint32_t last_notification_time = 0;

// ID de machine par défaut (sera remplacé par un ID unique)
#define DEFAULT_MACHINE_ID "nucleo_f411re"

void SupervisionService_Init(void) {
  if (is_initialized) {
    return;
  }
  
  // Utiliser l'ID de machine par défaut
  strncpy(machine_id, DEFAULT_MACHINE_ID, SUPERVISION_MAX_MACHINE_ID_LENGTH - 1);
  machine_id[SUPERVISION_MAX_MACHINE_ID_LENGTH - 1] = '\0';
  
  is_initialized = true;
  printf("[SUPERVISION] Service initialized with machine ID: %s\n", machine_id);
}

void SupervisionService_SendErrorNotification(SupervisionErrorType error_type, const char* message) {
  if (!is_initialized) {
    SupervisionService_Init();
  }
  
  if (!SupervisionService_ShouldSendNotification()) {
    printf("[SUPERVISION] Notification skipped due to cooldown period\n");
    return;
  }
  
  SupervisionEvent event;
  SupervisionService_GenerateErrorId(event.error_id, sizeof(event.error_id));
  strncpy(event.machine_id, machine_id, sizeof(event.machine_id) - 1);
  event.machine_id[sizeof(event.machine_id) - 1] = '\0';
  event.error_type = error_type;
  strncpy(event.message, message, sizeof(event.message) - 1);
  event.message[sizeof(event.message) - 1] = '\0';
  event.timestamp = HAL_GetTick();
  
  SupervisionService_SendErrorNotificationEvent(&event);
}

void SupervisionService_SendErrorNotificationEvent(const SupervisionEvent* event) {
  if (!is_initialized) {
    SupervisionService_Init();
  }
  
  if (!SupervisionService_ShouldSendNotification()) {
    printf("[SUPERVISION] Notification skipped due to cooldown period\n");
    return;
  }
  
  // Construire le payload JSON
  char json_payload[512];
  snprintf(json_payload, sizeof(json_payload),
    "{\"error_id\":\"%s\",\"machine_id\":\"%s\",\"error_type\":\"%s\",\"message\":\"%s\"}",
    event->error_id,
    event->machine_id,
    SupervisionService_ErrorTypeToString(event->error_type),
    event->message
  );
  
  printf("[SUPERVISION] Sending error notification:\n");
  printf("  Error ID: %s\n", event->error_id);
  printf("  Machine ID: %s\n", event->machine_id);
  printf("  Error Type: %s\n", SupervisionService_ErrorTypeToString(event->error_type));
  printf("  Message: %s\n", event->message);
  printf("  Payload: %s\n", json_payload);
  
  // Envoyer la notification via UART à l'ESP32
  // Format: SUPERVISION_ERROR:<json_payload>
  char uart_message[512];
  // Vérifier que le JSON ne dépasse pas la taille disponible
  size_t prefix_len = strlen("SUPERVISION_ERROR:");
  if (strlen(json_payload) + prefix_len < sizeof(uart_message)) {
    snprintf(uart_message, sizeof(uart_message), "SUPERVISION_ERROR:%s", json_payload);
  } else {
    // Tronquer le message si nécessaire
    size_t max_json_len = sizeof(uart_message) - prefix_len - 1;
    snprintf(uart_message, sizeof(uart_message), "SUPERVISION_ERROR:%.*s", 
             (int)max_json_len, json_payload);
  }
  
  // Utiliser le service de communication ESP pour envoyer le message
  EspComm_SendLine(uart_message);
  
  // Mettre à jour le timestamp de la dernière notification
  last_notification_time = HAL_GetTick();
  
  printf("[SUPERVISION] Error notification sent via UART\n");
}

void SupervisionService_GenerateErrorId(char* error_id, size_t max_len) {
  uint32_t timestamp = HAL_GetTick();
  uint32_t random_val = rand(); // Utiliser rand() pour la simplicité
  
  snprintf(error_id, max_len, "err_%08lx_%08lx", timestamp, random_val);
}

void SupervisionService_GetMachineId(char* machine_id_out, size_t max_len) {
  if (!is_initialized) {
    SupervisionService_Init();
  }
  strncpy(machine_id_out, machine_id, max_len - 1);
  machine_id_out[max_len - 1] = '\0';
}

void SupervisionService_SetMachineId(const char* new_machine_id) {
  strncpy(machine_id, new_machine_id, SUPERVISION_MAX_MACHINE_ID_LENGTH - 1);
  machine_id[SUPERVISION_MAX_MACHINE_ID_LENGTH - 1] = '\0';
  printf("[SUPERVISION] Machine ID set to: %s\n", machine_id);
}

const char* SupervisionService_ErrorTypeToString(SupervisionErrorType error_type) {
  switch (error_type) {
    case SUPERVISION_ERROR_WATCHDOG_TIMEOUT:
      return "WATCHDOG_TIMEOUT";
    case SUPERVISION_ERROR_WATCHDOG_RESET:
      return "WATCHDOG_RESET";
    case SUPERVISION_ERROR_TASK_HANG:
      return "TASK_HANG";
    case SUPERVISION_ERROR_MEMORY_LOW:
      return "MEMORY_LOW";
    case SUPERVISION_ERROR_CRITICAL_SERVICE_FAILURE:
      return "CRITICAL_SERVICE_FAILURE";
    case SUPERVISION_ERROR_HARDWARE_FAULT:
      return "HARDWARE_FAULT";
    case SUPERVISION_ERROR_SYSTEM_CRASH:
      return "SYSTEM_CRASH";
    case SUPERVISION_ERROR_MOTOR_FAILURE:
      return "MOTOR_FAILURE";
    case SUPERVISION_ERROR_SENSOR_FAILURE:
      return "SENSOR_FAILURE";
    default:
      return "UNKNOWN_ERROR";
  }
}

bool SupervisionService_ShouldSendNotification(void) {
  uint32_t current_time = HAL_GetTick();
  if (current_time - last_notification_time < SUPERVISION_NOTIFICATION_COOLDOWN_MS) {
    return false;
  }
  return true;
}
