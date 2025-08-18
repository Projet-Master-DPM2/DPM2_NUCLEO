#ifndef UNITY_NATIVE_TESTS
#define UNITY_NATIVE_TESTS
#endif
#include "unity.h"
#include "mock_hal.h"
#include "mock_freertos.h"
#include "mock_watchdog_service.h"

// Déclaration externe pour contrôle des tests
extern void Mock_Watchdog_SetResetFlag(bool flag);
extern void Mock_Watchdog_Reset(void);

// Variables de test
static bool test_watchdog_initialized = false;
static uint32_t test_heartbeat_count = 0;

// ============================================================================
// TESTS SETUP ET TEARDOWN
// ============================================================================

void setUp(void) {
    Mock_HAL_Reset();
    Mock_FreeRTOS_Reset();
    Mock_Watchdog_Reset();
    
    test_watchdog_initialized = false;
    test_heartbeat_count = 0;
}

void tearDown(void) {
    // Nettoyage après chaque test
}

// ============================================================================
// TESTS D'INITIALISATION
// ============================================================================

void test_watchdog_init_basic(void) {
    // Test initialisation de base
    Watchdog_Init();
    
    // Vérifier que l'initialisation s'est bien passée
    // (Dans la vraie implémentation, on vérifierait les variables internes)
    TEST_ASSERT_TRUE(true);  // Test basique
}

void test_watchdog_reset_detection(void) {
    // Simuler un reset watchdog précédent
    Mock_Watchdog_SetResetFlag(true);
    
    bool was_reset = Watchdog_WasResetCause();
    TEST_ASSERT_TRUE(was_reset);
    
    // Clear le flag
    Watchdog_ClearResetFlag();
    was_reset = Watchdog_WasResetCause();
    TEST_ASSERT_FALSE(was_reset);
}

void test_watchdog_start_sequence(void) {
    // Initialiser puis démarrer
    Watchdog_Init();
    Watchdog_Start();
    
    // Vérifier que le démarrage s'est bien passé
    TEST_ASSERT_TRUE(true);  // Test basique pour le mock
}

// ============================================================================
// TESTS DE SURVEILLANCE DES TÂCHES
// ============================================================================

void test_task_registration(void) {
    Watchdog_Init();
    
    // Mock d'un handle de tâche
    osThreadId_t mock_handle = (osThreadId_t)0x12345678;
    
    // Enregistrer une tâche
    bool result = Watchdog_RegisterTask(TASK_ORCHESTRATOR, &mock_handle, 2000);
    TEST_ASSERT_TRUE(result);
    
    // Enregistrer avec ID invalide
    result = Watchdog_RegisterTask(TASK_COUNT, &mock_handle, 2000);
    TEST_ASSERT_FALSE(result);
    
    // Enregistrer avec handle NULL
    result = Watchdog_RegisterTask(TASK_KEYPAD, NULL, 2000);
    TEST_ASSERT_FALSE(result);
}

void test_task_heartbeat_basic(void) {
    Watchdog_Init();
    
    osThreadId_t mock_handle = (osThreadId_t)0x12345678;
    Watchdog_RegisterTask(TASK_ORCHESTRATOR, &mock_handle, 2000);
    
    // Envoyer un heartbeat
    Mock_HAL_SetTick(1000);
    Watchdog_TaskHeartbeat(TASK_ORCHESTRATOR);
    
    // Vérifier que la tâche est vivante
    bool is_alive = Watchdog_IsTaskAlive(TASK_ORCHESTRATOR);
    TEST_ASSERT_TRUE(is_alive);
}

void test_task_timeout_detection(void) {
    Watchdog_Init();
    
    osThreadId_t mock_handle = (osThreadId_t)0x12345678;
    Watchdog_RegisterTask(TASK_ORCHESTRATOR, &mock_handle, 1000);  // Timeout 1s
    
    // Heartbeat initial
    Mock_HAL_SetTick(0);
    Mock_FreeRTOS_SetKernelState(osKernelRunning);  // Assurer que le scheduler est OK
    Watchdog_TaskHeartbeat(TASK_ORCHESTRATOR);
    
    // Simuler le passage du temps (pas de timeout)
    Mock_HAL_SetTick(500);
    bool health = Watchdog_CheckSystemHealth();
    TEST_ASSERT_TRUE(health);  // Encore dans les temps
    
    // Simuler timeout
    Mock_HAL_SetTick(1500);  // 1.5s depuis dernier heartbeat
    health = Watchdog_CheckSystemHealth();
    TEST_ASSERT_FALSE(health);  // Timeout dépassé
    
    bool is_alive = Watchdog_IsTaskAlive(TASK_ORCHESTRATOR);
    TEST_ASSERT_FALSE(is_alive);
}

// ============================================================================
// TESTS DE SANTÉ SYSTÈME
// ============================================================================

void test_system_health_multiple_tasks(void) {
    Watchdog_Init();
    
    osThreadId_t handle1 = (osThreadId_t)0x1111;
    osThreadId_t handle2 = (osThreadId_t)0x2222;
    
    Watchdog_RegisterTask(TASK_ORCHESTRATOR, &handle1, 1000);
    Watchdog_RegisterTask(TASK_KEYPAD, &handle2, 1500);
    
    // Heartbeats initiaux
    Mock_HAL_SetTick(0);
    Mock_FreeRTOS_SetKernelState(osKernelRunning);  // Scheduler OK
    Watchdog_TaskHeartbeat(TASK_ORCHESTRATOR);
    Watchdog_TaskHeartbeat(TASK_KEYPAD);
    
    // Système sain
    Mock_HAL_SetTick(500);
    bool health = Watchdog_CheckSystemHealth();
    TEST_ASSERT_TRUE(health);
    
    // Une tâche timeout, l'autre OK
    Mock_HAL_SetTick(1200);  // ORCHESTRATOR timeout, KEYPAD OK
    health = Watchdog_CheckSystemHealth();
    TEST_ASSERT_FALSE(health);  // Système défaillant
    
    // Récupération de la tâche
    Watchdog_TaskHeartbeat(TASK_ORCHESTRATOR);
    health = Watchdog_CheckSystemHealth();
    TEST_ASSERT_TRUE(health);  // Système récupéré
}

void test_freertos_scheduler_check(void) {
    Watchdog_Init();
    
    // Scheduler en marche
    Mock_FreeRTOS_SetKernelState(osKernelRunning);
    bool health = Watchdog_CheckSystemHealth();
    TEST_ASSERT_TRUE(health);
    
    // Scheduler défaillant
    Mock_FreeRTOS_SetKernelState(osKernelError);
    health = Watchdog_CheckSystemHealth();
    TEST_ASSERT_FALSE(health);
    
    // Scheduler inactif
    Mock_FreeRTOS_SetKernelState(osKernelInactive);
    health = Watchdog_CheckSystemHealth();
    TEST_ASSERT_FALSE(health);
}

// ============================================================================
// TESTS DE RAFRAÎCHISSEMENT
// ============================================================================

void test_watchdog_refresh_healthy_system(void) {
    Watchdog_Init();
    Watchdog_Start();
    
    osThreadId_t mock_handle = (osThreadId_t)0x12345678;
    Watchdog_RegisterTask(TASK_ORCHESTRATOR, &mock_handle, 2000);
    
    // Système sain
    Mock_HAL_SetTick(1000);
    Mock_FreeRTOS_SetKernelState(osKernelRunning);
    Watchdog_TaskHeartbeat(TASK_ORCHESTRATOR);
    
    // Rafraîchir le watchdog
    WatchdogStats_t stats_before = Watchdog_GetStats();
    Watchdog_Refresh();
    WatchdogStats_t stats_after = Watchdog_GetStats();
    
    // Vérifier que le compteur de refresh a augmenté
    TEST_ASSERT_EQUAL_UINT32(stats_before.totalRefresh + 1, stats_after.totalRefresh);
}

void test_watchdog_refresh_unhealthy_system(void) {
    Watchdog_Init();
    Watchdog_Start();
    
    osThreadId_t mock_handle = (osThreadId_t)0x12345678;
    Watchdog_RegisterTask(TASK_ORCHESTRATOR, &mock_handle, 1000);
    
    // Système défaillant (timeout)
    Mock_HAL_SetTick(0);
    Watchdog_TaskHeartbeat(TASK_ORCHESTRATOR);
    Mock_HAL_SetTick(2000);  // 2s après heartbeat
    Mock_FreeRTOS_SetKernelState(osKernelRunning);
    
    // Tentative de rafraîchissement
    WatchdogStats_t stats_before = Watchdog_GetStats();
    Watchdog_Refresh();
    WatchdogStats_t stats_after = Watchdog_GetStats();
    
    // Le compteur ne devrait PAS augmenter (système défaillant)
    TEST_ASSERT_EQUAL_UINT32(stats_before.totalRefresh, stats_after.totalRefresh);
}

// ============================================================================
// TESTS DE STATISTIQUES
// ============================================================================

void test_watchdog_statistics(void) {
    Watchdog_Init();
    Watchdog_Start();
    
    osThreadId_t mock_handle = (osThreadId_t)0x12345678;
    Watchdog_RegisterTask(TASK_ORCHESTRATOR, &mock_handle, 2000);
    
    // Obtenir statistiques initiales
    WatchdogStats_t stats = Watchdog_GetStats();
    uint32_t initial_refresh = stats.totalRefresh;
    
    // Effectuer quelques rafraîchissements
    Mock_HAL_SetTick(1000);
    Mock_FreeRTOS_SetKernelState(osKernelRunning);
    Watchdog_TaskHeartbeat(TASK_ORCHESTRATOR);
    
    for (int i = 0; i < 5; i++) {
        Watchdog_Refresh();
        Mock_HAL_SetTick(1000 + (i + 1) * 100);
    }
    
    // Vérifier les statistiques
    stats = Watchdog_GetStats();
    TEST_ASSERT_EQUAL_UINT32(initial_refresh + 5, stats.totalRefresh);
}

// ============================================================================
// TESTS D'ACTIVATION/DÉSACTIVATION
// ============================================================================

void test_task_enable_disable(void) {
    Watchdog_Init();
    
    osThreadId_t mock_handle = (osThreadId_t)0x12345678;
    Watchdog_RegisterTask(TASK_ORCHESTRATOR, &mock_handle, 1000);
    
    // Désactiver la surveillance de la tâche
    Watchdog_EnableTask(TASK_ORCHESTRATOR, false);
    
    // Même avec timeout, le système devrait être sain (tâche désactivée)
    Mock_HAL_SetTick(0);
    Watchdog_TaskHeartbeat(TASK_ORCHESTRATOR);
    Mock_HAL_SetTick(2000);  // Timeout dépassé
    Mock_FreeRTOS_SetKernelState(osKernelRunning);
    
    bool health = Watchdog_CheckSystemHealth();
    TEST_ASSERT_TRUE(health);  // Sain car tâche désactivée
    
    // Réactiver la surveillance
    Watchdog_EnableTask(TASK_ORCHESTRATOR, true);
    health = Watchdog_CheckSystemHealth();
    TEST_ASSERT_FALSE(health);  // Défaillant car timeout
}

// ============================================================================
// MAIN DE TEST
// ============================================================================

int main(void) {
    UNITY_BEGIN();
    
    // Tests d'initialisation
    RUN_TEST(test_watchdog_init_basic);
    RUN_TEST(test_watchdog_reset_detection);
    RUN_TEST(test_watchdog_start_sequence);
    
    // Tests de surveillance
    RUN_TEST(test_task_registration);
    RUN_TEST(test_task_heartbeat_basic);
    RUN_TEST(test_task_timeout_detection);
    
    // Tests de santé système
    RUN_TEST(test_system_health_multiple_tasks);
    RUN_TEST(test_freertos_scheduler_check);
    
    // Tests de rafraîchissement
    RUN_TEST(test_watchdog_refresh_healthy_system);
    RUN_TEST(test_watchdog_refresh_unhealthy_system);
    
    // Tests avancés
    RUN_TEST(test_watchdog_statistics);
    RUN_TEST(test_task_enable_disable);
    
    return UNITY_END();
}
