/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "lcd_service.h"
#include "blink_led.h"
#include "send_uart.h"
#include "keypad_service.h"
#include "motor_service.h"
#include "orchestrator_service.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
osThreadId_t blinkLEDHandle;
osThreadId_t sendUARTHandle;
osThreadId_t lcdTaskHandle;
osThreadId_t keypadTaskHandle;
osThreadId_t motorTaskHandle;
osThreadId_t orchestratorTaskHandle;

osMessageQueueId_t keypadEventQueueHandle;
osMessageQueueId_t orchestratorEventQueueHandle;
osMessageQueueId_t lcdMessageQueueHandle;

const osThreadAttr_t blinkLED_attributes = {
  .name = "blinkLED",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t sendUART_attributes = {
  .name = "sendUART",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

const osThreadAttr_t lcdTask_attributes = {
  .name = "lcdTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t keypadTask_attributes = {
  .name = "keypadTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t motorTask_attributes = {
  .name = "motorTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t orchestratorTask_attributes = {
  .name = "orchestratorTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};

const osMessageQueueAttr_t keypadEventQueue_attributes = {
  .name = "keypadEventQueue"
};
const osMessageQueueAttr_t orchestratorEventQueue_attributes = {
  .name = "orchestratorEventQueue"
};
const osMessageQueueAttr_t lcdMessageQueue_attributes = {
  .name = "lcdMessageQueue"
};
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
extern void StartTaskBlinkLED(void *argument);
extern void StartTaskSendUART(void *argument);
extern void StartTaskLCD(void *argument);
extern void StartTaskKeypad(void *argument);
extern void StartTaskMotorService(void *argument);
extern void StartTaskOrchestrator(void *argument);

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
	printf("\r\nCreating tasks...\r\n");
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  keypadEventQueueHandle = osMessageQueueNew(8, sizeof(KeypadEvent), &keypadEventQueue_attributes);
  orchestratorEventQueueHandle = osMessageQueueNew(8, sizeof(OrchestratorEvent), &orchestratorEventQueue_attributes);
  lcdMessageQueueHandle = osMessageQueueNew(4, sizeof(LcdMessage), &lcdMessageQueue_attributes);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  blinkLEDHandle    = osThreadNew(StartTaskBlinkLED, NULL, &blinkLED_attributes);
  //sendUARTHandle    = osThreadNew(StartTaskSendUART, NULL, &sendUART_attributes);
  lcdTaskHandle     = osThreadNew(StartTaskLCD, NULL, &lcdTask_attributes);
  keypadTaskHandle  = osThreadNew(StartTaskKeypad, NULL, &keypadTask_attributes);
  motorTaskHandle  = osThreadNew(StartTaskMotorService, NULL, &motorTask_attributes);
  orchestratorTaskHandle = osThreadNew(StartTaskOrchestrator, NULL, &orchestratorTask_attributes);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

