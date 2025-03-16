/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "string.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "edconfig.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct EmergencyEvent_t
{
    int event_id;
} EmergencyEvent_t;

typedef struct LogEvent_t
{
    int event_id;
    int log_type;
    char msg[LOG_LEN];
} LogEvent_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
#if defined ( __ICCARM__ ) /*!< IAR Compiler */
#pragma location=0x2004c000
ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
#pragma location=0x2004c0a0
ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */

#elif defined ( __CC_ARM )  /* MDK ARM Compiler */

__attribute__((at(0x2004c000))) ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
__attribute__((at(0x2004c0a0))) ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */

#elif defined ( __GNUC__ ) /* GNU Compiler */

ETH_DMADescTypeDef DMARxDscrTab[ETH_RX_DESC_CNT] __attribute__((section(".RxDecripSection"))); /* Ethernet Rx DMA Descriptors */
ETH_DMADescTypeDef DMATxDscrTab[ETH_TX_DESC_CNT] __attribute__((section(".TxDecripSection")));   /* Ethernet Tx DMA Descriptors */
#endif

ETH_TxPacketConfig TxConfig;

ETH_HandleTypeDef heth;

I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart3;

PCD_HandleTypeDef hpcd_USB_OTG_FS;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* USER CODE BEGIN PV */
QueueHandle_t xEventQueue;
QueueHandle_t xPoliceQueue;
QueueHandle_t xAmbulanceQueue;
QueueHandle_t xFireDeptQueue;
QueueHandle_t xLogQueue;

SemaphoreHandle_t xSemPoliceCabs;
SemaphoreHandle_t xSemAmbulances;
SemaphoreHandle_t xSemFiretrucks;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ETH_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);
void StartDefaultTask(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// For printf through uart3
int __io_putchar(int ch)
{
	HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, 0xFFFF);
	return ch;
}

void Task_Generator(void *pvParameters)
{
    srand(time(NULL));

    for (;;)
    {
        EmergencyEvent_t event;
        event.event_id = rand() % 3 + 1;

        LogEvent_t log;
        log.event_id = event.event_id;
        log.log_type = LOG_CREATE;
        strcpy(log.msg, "[Generator] Created event");

        xQueueSend(xEventQueue, &event, portMAX_DELAY);
        xQueueSend(xLogQueue, &log, portMAX_DELAY);

        int delay = rand() % MAX_DELAY + MIN_DELAY;
        vTaskDelay(pdMS_TO_TICKS(delay));
    }

}

void Task_Dispatcher(void *pvParameters)
{
    EmergencyEvent_t event;

    for (;;)
    {
        if (xQueueReceive(xEventQueue, &event, portMAX_DELAY) == pdPASS)
        {
            LogEvent_t log;
            log.event_id = event.event_id;

            switch (event.event_id)
            {
            case CODE_PLC:
                xQueueSend(xPoliceQueue, &event, portMAX_DELAY);
                log.log_type = LOG_SENT;
                strcpy(log.msg, "[Dispatcher] Sent event to police");
                break;

            case CODE_AMB:
                xQueueSend(xAmbulanceQueue, &event, portMAX_DELAY);
                log.log_type = LOG_SENT;
                strcpy(log.msg, "[Dispatcher] Sent event to first aid");
                break;

            case CODE_FIR:
                xQueueSend(xFireDeptQueue, &event, portMAX_DELAY);
                log.log_type = LOG_SENT;
                strcpy(log.msg, "[Dispatcher] Sent event to fire department");
                break;

            default:
                log.log_type = LOG_ERR;
                strcpy(log.msg, "[Dispatcher] Unknown event type");
                break;
            }

            xQueueSend(xLogQueue, &log, portMAX_DELAY);
        }
    }

}

void Task_Police(void *pvParameters)
{
    EmergencyEvent_t event;

    for(;;)
    {
        if(xQueueReceive(xPoliceQueue, &event, portMAX_DELAY) == pdPASS)
        {
            LogEvent_t log;
            log.event_id = event.event_id;
            log.log_type = LOG_PLC_RCV;
            strcpy(log.msg, "[Police] Received event");
            xQueueSend(xLogQueue, &log, portMAX_DELAY);

            if(xSemaphoreTake(xSemPoliceCabs, portMAX_DELAY) == pdPASS)
            {
                vTaskDelay(pdMS_TO_TICKS(HNDL_DELAY));

                log.log_type = LOG_PLC_HNDL;
                strcpy(log.msg, "[Police] Finihed handling event");
                xQueueSend(xLogQueue, &log, portMAX_DELAY);

                xSemaphoreGive(xSemPoliceCabs);
            }
            else if (xSemaphoreTake(xSemAmbulances, portMAX_DELAY) == pdPASS)
            {
                vTaskDelay(pdMS_TO_TICKS(HNDL_DELAY));

                log.log_type = LOG_PLC_HNDL;
                strcpy(log.msg, "[Police] Finihed handling event WITH BORROWED AMBULANCE");
                xQueueSend(xLogQueue, &log, portMAX_DELAY);

                xSemaphoreGive(xSemAmbulances);
            }
            else if (xSemaphoreTake(xSemFiretrucks, portMAX_DELAY) == pdPASS)
            {
                vTaskDelay(pdMS_TO_TICKS(HNDL_DELAY));

                log.log_type = LOG_PLC_HNDL;
                strcpy(log.msg, "[Police] Finihed handling event WITH BORROWED FIRETRUCK");
                xQueueSend(xLogQueue, &log, portMAX_DELAY);

                xSemaphoreGive(xSemFiretrucks);
            }
        }
    }
}

void Task_Ambulance(void *pvParameters)
{
    EmergencyEvent_t event;

    for(;;)
    {
        if(xQueueReceive(xAmbulanceQueue, &event, portMAX_DELAY) == pdPASS)
        {
            LogEvent_t log;
            log.event_id = event.event_id;
            log.log_type = LOG_AMB_RCV;
            strcpy(log.msg, "[Ambulance] Received event");
            xQueueSend(xLogQueue, &log, portMAX_DELAY);

            if(xSemaphoreTake(xSemAmbulances, portMAX_DELAY) == pdPASS)
            {
                vTaskDelay(pdMS_TO_TICKS(HNDL_DELAY));

                log.log_type = LOG_AMB_HNDL;
                strcpy(log.msg, "[Ambulance] Finihed handling event");
                xQueueSend(xLogQueue, &log, portMAX_DELAY);

                xSemaphoreGive(xSemAmbulances);
            }
            else if (xSemaphoreTake(xSemPoliceCabs, portMAX_DELAY) == pdPASS)
            {
                vTaskDelay(pdMS_TO_TICKS(HNDL_DELAY));

                log.log_type = LOG_AMB_HNDL;
                strcpy(log.msg, "[Ambulance] Finihed handling event WITH BORROWED POLICE CAB");
                xQueueSend(xLogQueue, &log, portMAX_DELAY);

                xSemaphoreGive(xSemPoliceCabs);
            }
            else if (xSemaphoreTake(xSemFiretrucks, portMAX_DELAY) == pdPASS)
            {
                vTaskDelay(pdMS_TO_TICKS(HNDL_DELAY));

                log.log_type = LOG_AMB_HNDL;
                strcpy(log.msg, "[Ambulance] Finihed handling event WITH BORROWED FIRETRUCK");
                xQueueSend(xLogQueue, &log, portMAX_DELAY);

                xSemaphoreGive(xSemFiretrucks);
            }
        }
    }
}

void Task_FireDepartment(void *pvParameters)
{
    EmergencyEvent_t event;

    for(;;)
    {
        if(xQueueReceive(xFireDeptQueue, &event, portMAX_DELAY) == pdPASS)
        {
            LogEvent_t log;
            log.event_id = event.event_id;
            log.log_type = LOG_FIR_RCV;
            strcpy(log.msg, "[Fire Dept] Received event");
            xQueueSend(xLogQueue, &log, portMAX_DELAY);

            if(xSemaphoreTake(xSemFiretrucks, portMAX_DELAY) == pdPASS)
            {
                vTaskDelay(pdMS_TO_TICKS(HNDL_DELAY));

                log.log_type = LOG_FIR_HNDL;
                strcpy(log.msg, "[Fire Dept] Finihed handling event");
                xQueueSend(xLogQueue, &log, portMAX_DELAY);

                xSemaphoreGive(xSemFiretrucks);
            }
            else if(xSemaphoreTake(xSemAmbulances, portMAX_DELAY) == pdPASS)
            {
                vTaskDelay(pdMS_TO_TICKS(HNDL_DELAY));

                log.log_type = LOG_FIR_HNDL;
                strcpy(log.msg, "[Fire Dept] Finihed handling event WITH BORROWED AMBULANCE");
                xQueueSend(xLogQueue, &log, portMAX_DELAY);

                xSemaphoreGive(xSemAmbulances);
            }
            else if (xSemaphoreTake(xSemPoliceCabs, portMAX_DELAY) == pdPASS)
            {
                vTaskDelay(pdMS_TO_TICKS(HNDL_DELAY));

                log.log_type = LOG_FIR_HNDL;
                strcpy(log.msg, "[Fire Dept] Finished handling event WITH BORROWED POLICE CAB");
                xQueueSend(xLogQueue, &log, portMAX_DELAY);

                xSemaphoreGive(xSemPoliceCabs);
            }
        }
    }
}

void Task_Logger(void *pvParameters)
{
    LogEvent_t log;

    for (;;)
    {
        if (xQueueReceive(xLogQueue, &log, portMAX_DELAY) == pdPASS)
        {
            printf("Event type %d: %s [status: %d]\r\n", log.event_id, log.msg, log.log_type);
        }
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ETH_Init();
  MX_I2C1_Init();
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  xSemPoliceCabs = xSemaphoreCreateCounting(N_PCR, N_PCR);
  xSemAmbulances = xSemaphoreCreateCounting(N_AMB, N_AMB);
  xSemFiretrucks = xSemaphoreCreateCounting(N_FTR, N_FTR);
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  xEventQueue = xQueueCreate(QUEUE_SIZE, sizeof(EmergencyEvent_t));
      if (xEventQueue == NULL)
      {
          printf("Failed to create event queue\r\n");
          while (1);
      }

      xPoliceQueue = xQueueCreate(QUEUE_SIZE, sizeof(EmergencyEvent_t));
      if (xPoliceQueue == NULL)
      {
    	  printf("Failed to create police queue\r\n");
    	  while (1);
      }

      xAmbulanceQueue = xQueueCreate(QUEUE_SIZE, sizeof(EmergencyEvent_t));
      if (xAmbulanceQueue == NULL)
      {
    	  printf("Failed to create ambulance queue\r\n");
    	  while (1);
      }

      xFireDeptQueue = xQueueCreate(QUEUE_SIZE, sizeof(EmergencyEvent_t));
      if (xFireDeptQueue == NULL)
      {
    	  printf("Failed to create fire department queue\r\n");
    	  while (1);
      }

      xLogQueue = xQueueCreate(QUEUE_SIZE, sizeof(LogEvent_t));
      if (xLogQueue == NULL)
      {
    	  printf("Failed to create log queue\r\n");
    	  while (1);
      }
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */

  if(xTaskCreate(Task_Generator, "Generator", configMINIMAL_STACK_SIZE, NULL, PR_GEN, NULL) != pdPASS)
  {
	  printf("Failed to create Generator task\r\n");
	  while (1);
  }

  if(xTaskCreate(Task_Dispatcher, "Dispatcher", configMINIMAL_STACK_SIZE, NULL, PR_DIS, NULL) != pdPASS)
  {
  	  printf("Failed to create Dispatcher task\r\n");
  	  while (1);
  }

  if(xTaskCreate(Task_Police, "Police", configMINIMAL_STACK_SIZE, NULL, PR_PLC, NULL) != pdPASS)
    {
    	  printf("Failed to create Police task\r\n");
    	  while (1);
    }

  if(xTaskCreate(Task_Ambulance, "Ambulance", configMINIMAL_STACK_SIZE, NULL, PR_AMB, NULL) != pdPASS)
    {
    	  printf("Failed to create Ambulance task\r\n");
    	  while (1);
    }

  if(xTaskCreate(Task_FireDepartment, "Fire Department", configMINIMAL_STACK_SIZE, NULL, PR_FIR, NULL) != pdPASS)
    {
    	  printf("Failed to create Fire Department task\r\n");
    	  while (1);
    }

  if(xTaskCreate(Task_Logger, "Logger", configMINIMAL_STACK_SIZE, NULL, PR_LOG, NULL) != pdPASS)
    {
    	  printf("Failed to create Logger task\r\n");
    	  while (1);
    }

  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  printf("Scheduler failed \r\n");
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ETH Initialization Function
  * @param None
  * @retval None
  */
static void MX_ETH_Init(void)
{

  /* USER CODE BEGIN ETH_Init 0 */

  /* USER CODE END ETH_Init 0 */

   static uint8_t MACAddr[6];

  /* USER CODE BEGIN ETH_Init 1 */

  /* USER CODE END ETH_Init 1 */
  heth.Instance = ETH;
  MACAddr[0] = 0x00;
  MACAddr[1] = 0x80;
  MACAddr[2] = 0xE1;
  MACAddr[3] = 0x00;
  MACAddr[4] = 0x00;
  MACAddr[5] = 0x00;
  heth.Init.MACAddr = &MACAddr[0];
  heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
  heth.Init.TxDesc = DMATxDscrTab;
  heth.Init.RxDesc = DMARxDscrTab;
  heth.Init.RxBuffLen = 1524;

  /* USER CODE BEGIN MACADDRESS */

  /* USER CODE END MACADDRESS */

  if (HAL_ETH_Init(&heth) != HAL_OK)
  {
    Error_Handler();
  }

  memset(&TxConfig, 0 , sizeof(ETH_TxPacketConfig));
  TxConfig.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
  TxConfig.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
  TxConfig.CRCPadCtrl = ETH_CRC_PAD_INSERT;
  /* USER CODE BEGIN ETH_Init 2 */

  /* USER CODE END ETH_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00808CD2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief USB_OTG_FS Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_OTG_FS_PCD_Init(void)
{

  /* USER CODE BEGIN USB_OTG_FS_Init 0 */

  /* USER CODE END USB_OTG_FS_Init 0 */

  /* USER CODE BEGIN USB_OTG_FS_Init 1 */

  /* USER CODE END USB_OTG_FS_Init 1 */
  hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
  hpcd_USB_OTG_FS.Init.dev_endpoints = 6;
  hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_OTG_FS.Init.Sof_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.vbus_sensing_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_OTG_FS_Init 2 */

  /* USER CODE END USB_OTG_FS_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LD1_Pin|LD3_Pin|LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : USER_Btn_Pin */
  GPIO_InitStruct.Pin = USER_Btn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USER_Btn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD1_Pin LD3_Pin LD2_Pin */
  GPIO_InitStruct.Pin = LD1_Pin|LD3_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_OverCurrent_Pin */
  GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END 5 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
