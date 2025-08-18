#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

// Configuration Unity pour STM32 et tests natifs
#ifdef UNITY_NATIVE_TESTS
    // Configuration pour tests natifs (PC)
    #define UNITY_OUTPUT_CHAR(c)        putchar(c)
    #define UNITY_OUTPUT_FLUSH()        fflush(stdout)
    #define UNITY_OUTPUT_START()        
    #define UNITY_OUTPUT_COMPLETE()     
#else
    // Configuration pour tests embarqués (STM32)
    #include "stm32f4xx_hal.h"
    extern UART_HandleTypeDef huart2;
    
    #define UNITY_OUTPUT_CHAR(c)        HAL_UART_Transmit(&huart2, (uint8_t*)&c, 1, 100)
    #define UNITY_OUTPUT_FLUSH()        
    #define UNITY_OUTPUT_START()        
    #define UNITY_OUTPUT_COMPLETE()     
#endif

// Configuration générale Unity
#define UNITY_INCLUDE_64                1
#define UNITY_INCLUDE_FLOAT             1
#define UNITY_INCLUDE_DOUBLE            0
#define UNITY_SUPPORT_64                1

// Timeouts pour tests embarqués
#define UNITY_TEST_TIMEOUT_MS           5000
#define UNITY_FREERTOS_DELAY_MS         10

// Configuration des assertions étendues
#define UNITY_INCLUDE_PRINT_FORMATTED   1
#define UNITY_PRINT_EOL                 "\r\n"

// Gestion mémoire pour tests natifs
#ifdef UNITY_NATIVE_TESTS
    #include <stdlib.h>
    #define UNITY_MALLOC(size)          malloc(size)
    #define UNITY_FREE(ptr)             free(ptr)
#endif

// Macros spécifiques au projet DPM
#define TEST_ASSERT_STATE_EQUAL(expected, actual) \
    TEST_ASSERT_EQUAL_INT((int)(expected), (int)(actual))

#define TEST_ASSERT_WITHIN_TIMEOUT(expected_ms, actual_ms, tolerance_ms) \
    TEST_ASSERT_INT_WITHIN(tolerance_ms, expected_ms, actual_ms)

// Assurer que les macros sont disponibles
#ifndef TEST_ASSERT_STATE_EQUAL
#define TEST_ASSERT_STATE_EQUAL(expected, actual) TEST_ASSERT_EQUAL_INT((int)(expected), (int)(actual))
#endif

#endif // UNITY_CONFIG_H
