#include "mock_freertos.h"

#ifdef UNITY_NATIVE_TESTS

#include <stdlib.h>
#include <string.h>

// État du mock FreeRTOS
static struct {
    uint32_t current_tick;
    osKernelState_t kernel_state;
    uint32_t thread_count;
    uint32_t queue_count;
    uint32_t mutex_count;
    uint32_t delay_call_count;
    
    // Simulation des objets FreeRTOS
    struct {
        bool in_use;
        void (*func)(void*);
        void* argument;
        const osThreadAttr_t* attr;
    } threads[16];
    
    struct queue_state {
        bool in_use;
        uint32_t msg_count;
        uint32_t msg_size;
        void* messages[32];  // Buffer simple pour messages
        uint32_t head, tail, count;
    } queues[8];
    
    struct mutex_state {
        bool in_use;
        bool locked;
        osThreadId_t owner;
    } mutexes[8];
    
} mock_freertos = {
    .kernel_state = osKernelInactive,
    .current_tick = 0
};

// ============================================================================
// FONCTIONS KERNEL
// ============================================================================

osStatus_t osKernelInitialize(void) {
    mock_freertos.kernel_state = osKernelReady;
    return osOK;
}

osKernelState_t osKernelGetState(void) {
    return mock_freertos.kernel_state;
}

osStatus_t osKernelStart(void) {
    mock_freertos.kernel_state = osKernelRunning;
    return osOK;
}

uint32_t osKernelGetTickCount(void) {
    return mock_freertos.current_tick;
}

// ============================================================================
// FONCTIONS THREADS
// ============================================================================

osThreadId_t osThreadNew(void (*func)(void*), void* argument, const osThreadAttr_t* attr) {
    for (int i = 0; i < 16; i++) {
        if (!mock_freertos.threads[i].in_use) {
            mock_freertos.threads[i].in_use = true;
            mock_freertos.threads[i].func = func;
            mock_freertos.threads[i].argument = argument;
            mock_freertos.threads[i].attr = attr;
            mock_freertos.thread_count++;
            return (osThreadId_t)(uintptr_t)(i + 1);  // ID non-nul
        }
    }
    return NULL;
}

osStatus_t osThreadTerminate(osThreadId_t thread_id) {
    int index = (int)(uintptr_t)thread_id - 1;
    if (index >= 0 && index < 16 && mock_freertos.threads[index].in_use) {
        mock_freertos.threads[index].in_use = false;
        mock_freertos.thread_count--;
        return osOK;
    }
    return osErrorParameter;
}

osStatus_t osDelay(uint32_t ticks) {
    mock_freertos.delay_call_count++;
    mock_freertos.current_tick += ticks;
    return osOK;
}

// ============================================================================
// FONCTIONS MESSAGE QUEUES
// ============================================================================

osMessageQueueId_t osMessageQueueNew(uint32_t msg_count, uint32_t msg_size, const osMessageQueueAttr_t* attr) {
    for (int i = 0; i < 8; i++) {
        if (!mock_freertos.queues[i].in_use) {
            mock_freertos.queues[i].in_use = true;
            mock_freertos.queues[i].msg_count = msg_count;
            mock_freertos.queues[i].msg_size = msg_size;
            mock_freertos.queues[i].head = 0;
            mock_freertos.queues[i].tail = 0;
            mock_freertos.queues[i].count = 0;
            mock_freertos.queue_count++;
            return (osMessageQueueId_t)(uintptr_t)(i + 1);
        }
    }
    return NULL;
}

osStatus_t osMessageQueuePut(osMessageQueueId_t mq_id, const void* msg_ptr, uint8_t msg_prio, uint32_t timeout) {
    int index = (int)(uintptr_t)mq_id - 1;
    if (index >= 0 && index < 8 && mock_freertos.queues[index].in_use) {
        struct queue_state* queue = &mock_freertos.queues[index];
        
        if (queue->count >= queue->msg_count) {
            return osErrorResource;  // Queue pleine
        }
        
        // Allouer et copier le message
        queue->messages[queue->tail] = malloc(queue->msg_size);
        if (queue->messages[queue->tail] && msg_ptr) {
            memcpy(queue->messages[queue->tail], msg_ptr, queue->msg_size);
        }
        
        queue->tail = (queue->tail + 1) % 32;
        queue->count++;
        return osOK;
    }
    return osErrorParameter;
}

osStatus_t osMessageQueueGet(osMessageQueueId_t mq_id, void* msg_ptr, uint8_t* msg_prio, uint32_t timeout) {
    int index = (int)(uintptr_t)mq_id - 1;
    if (index >= 0 && index < 8 && mock_freertos.queues[index].in_use) {
        struct queue_state* queue = &mock_freertos.queues[index];
        
        if (queue->count == 0) {
            return osErrorResource;  // Queue vide
        }
        
        // Copier et libérer le message
        if (msg_ptr && queue->messages[queue->head]) {
            memcpy(msg_ptr, queue->messages[queue->head], queue->msg_size);
            free(queue->messages[queue->head]);
            queue->messages[queue->head] = NULL;
        }
        
        queue->head = (queue->head + 1) % 32;
        queue->count--;
        return osOK;
    }
    return osErrorParameter;
}

// ============================================================================
// FONCTIONS MUTEX
// ============================================================================

osMutexId_t osMutexNew(const osMutexAttr_t* attr) {
    for (int i = 0; i < 8; i++) {
        if (!mock_freertos.mutexes[i].in_use) {
            mock_freertos.mutexes[i].in_use = true;
            mock_freertos.mutexes[i].locked = false;
            mock_freertos.mutexes[i].owner = NULL;
            mock_freertos.mutex_count++;
            return (osMutexId_t)(uintptr_t)(i + 1);
        }
    }
    return NULL;
}

osStatus_t osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout) {
    int index = (int)(uintptr_t)mutex_id - 1;
    if (index >= 0 && index < 8 && mock_freertos.mutexes[index].in_use) {
        struct mutex_state* mutex = &mock_freertos.mutexes[index];
        
        if (mutex->locked) {
            return osErrorResource;  // Déjà verrouillé
        }
        
        mutex->locked = true;
        mutex->owner = (osThreadId_t)1;  // Thread fictif
        return osOK;
    }
    return osErrorParameter;
}

osStatus_t osMutexRelease(osMutexId_t mutex_id) {
    int index = (int)(uintptr_t)mutex_id - 1;
    if (index >= 0 && index < 8 && mock_freertos.mutexes[index].in_use) {
        struct mutex_state* mutex = &mock_freertos.mutexes[index];
        
        if (!mutex->locked) {
            return osErrorResource;  // Pas verrouillé
        }
        
        mutex->locked = false;
        mutex->owner = NULL;
        return osOK;
    }
    return osErrorParameter;
}

// ============================================================================
// FONCTIONS DE CONTRÔLE DES MOCKS
// ============================================================================

void Mock_FreeRTOS_Reset(void) {
    // Libérer les messages alloués
    for (int i = 0; i < 8; i++) {
        if (mock_freertos.queues[i].in_use) {
            for (int j = 0; j < 32; j++) {
                if (mock_freertos.queues[i].messages[j]) {
                    free(mock_freertos.queues[i].messages[j]);
                }
            }
        }
    }
    
    memset(&mock_freertos, 0, sizeof(mock_freertos));
    mock_freertos.kernel_state = osKernelInactive;
}

void Mock_FreeRTOS_SetTick(uint32_t tick) {
    mock_freertos.current_tick = tick;
}

void Mock_FreeRTOS_SetKernelState(osKernelState_t state) {
    mock_freertos.kernel_state = state;
}

void Mock_FreeRTOS_SimulateDelay(uint32_t ticks) {
    mock_freertos.current_tick += ticks;
}

// ============================================================================
// VÉRIFICATIONS DES MOCKS
// ============================================================================

uint32_t Mock_FreeRTOS_GetThreadCount(void) {
    return mock_freertos.thread_count;
}

uint32_t Mock_FreeRTOS_GetQueueCount(void) {
    return mock_freertos.queue_count;
}

uint32_t Mock_FreeRTOS_GetMutexCount(void) {
    return mock_freertos.mutex_count;
}

uint32_t Mock_FreeRTOS_GetDelayCallCount(void) {
    return mock_freertos.delay_call_count;
}

#endif // UNITY_NATIVE_TESTS
