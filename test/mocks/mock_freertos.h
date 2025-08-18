#ifndef MOCK_FREERTOS_H
#define MOCK_FREERTOS_H

#include <stdint.h>
#include <stdbool.h>

// Mock pour les tests natifs uniquement
#ifdef UNITY_NATIVE_TESTS

// Types FreeRTOS mockés
typedef enum {
    osOK = 0,
    osError = -1,
    osErrorTimeout = -2,
    osErrorResource = -3,
    osErrorParameter = -4
} osStatus_t;

typedef enum {
    osKernelInactive = 0,
    osKernelReady = 1,
    osKernelRunning = 2,
    osKernelLocked = 3,
    osKernelSuspended = 4,
    osKernelError = -1
} osKernelState_t;

typedef enum {
    osPriorityNone = 0,
    osPriorityIdle = 1,
    osPriorityLow = 8,
    osPriorityNormal = 24,
    osPriorityAboveNormal = 32,
    osPriorityHigh = 40,
    osPriorityRealtime = 48
} osPriority_t;

// Handles mockés
typedef void* osThreadId_t;
typedef void* osMessageQueueId_t;
typedef void* osMutexId_t;

// Structures mockées
typedef struct {
    const char* name;
    uint32_t stack_size;
    osPriority_t priority;
} osThreadAttr_t;

typedef struct {
    const char* name;
} osMessageQueueAttr_t;

typedef struct {
    const char* name;
} osMutexAttr_t;

// Constantes
#define osWaitForever 0xFFFFFFFFU

// Fonctions FreeRTOS mockées
osStatus_t osKernelInitialize(void);
osKernelState_t osKernelGetState(void);
osStatus_t osKernelStart(void);
uint32_t osKernelGetTickCount(void);

osThreadId_t osThreadNew(void (*func)(void*), void* argument, const osThreadAttr_t* attr);
osStatus_t osThreadTerminate(osThreadId_t thread_id);
osStatus_t osDelay(uint32_t ticks);

osMessageQueueId_t osMessageQueueNew(uint32_t msg_count, uint32_t msg_size, const osMessageQueueAttr_t* attr);
osStatus_t osMessageQueuePut(osMessageQueueId_t mq_id, const void* msg_ptr, uint8_t msg_prio, uint32_t timeout);
osStatus_t osMessageQueueGet(osMessageQueueId_t mq_id, void* msg_ptr, uint8_t* msg_prio, uint32_t timeout);

osMutexId_t osMutexNew(const osMutexAttr_t* attr);
osStatus_t osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout);
osStatus_t osMutexRelease(osMutexId_t mutex_id);

// Fonctions de contrôle des mocks
void Mock_FreeRTOS_Reset(void);
void Mock_FreeRTOS_SetTick(uint32_t tick);
void Mock_FreeRTOS_SetKernelState(osKernelState_t state);
void Mock_FreeRTOS_SimulateDelay(uint32_t ticks);

// Vérifications des mocks
uint32_t Mock_FreeRTOS_GetThreadCount(void);
uint32_t Mock_FreeRTOS_GetQueueCount(void);
uint32_t Mock_FreeRTOS_GetMutexCount(void);
uint32_t Mock_FreeRTOS_GetDelayCallCount(void);

#endif // UNITY_NATIVE_TESTS

#endif // MOCK_FREERTOS_H
