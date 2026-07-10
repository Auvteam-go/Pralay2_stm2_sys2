/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "dht11.h"
#include "ina260.h"
#include "leak_sensor.h"
#include "string.h"
#include "stddef.h"
#include "stdbool.h"
#include "stdlib.h"
#include "stdio.h"

helkkkksdfj skadf klkl;sf

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

#pragma pack(push, 1)
typedef struct {

	uint8_t header; //0xAA

	float temperature; // DHT11
	float humdity;
	float dhtvalid;

	float voltage;		// INA260
	float current;
	float power;
	uint8_t inaValid;

	uint16_t leakRaw;		// LEAK SENSOR
	uint8_t leak_Detected;
	uint8_t leakValid;

	uint32_t seqNumber;
	uint8_t checksum;
	uint8_t footer; // 0xBB

}TelemetryPacket_t;
#pragma pack(pop)

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define DHT11_PERIOD_MS 1000  //1 sec
#define INA260_PERIOD_MS 200
#define LEAK_PERIO_MS 100
#define UART_PERIOD_MS 500

#define TELEMETRY_HEADER_BYTE 0XAA
#define TELEMETERY_FOOTER_BYTE 0X55


static osThreadId_t dht11TaskHandle;
static osThreadId_t ina260TaskHandle;
static osThreadId_t leakTaskHandle;
static osThreadId_t uartaskHandle;




/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_tx;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* USER CODE BEGIN PV */
static osSemaphoreId_t s_txDonesem;
static osMutexId_t g_dhtMutex;
static osMutexId_t g_inaMutex;
static osMutexId_t g_leakMutex;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
void StartDefaultTask(void *argument);

/* USER CODE BEGIN PFP */

static void DHT11Task(void *argument);
static void INA260Task(void * argument);
static void LeakTask(void *argument);
static void UartTask(void *argument);
static void App_CreateTaskAndSync(void);
static uint8_t Telemetery_Checksum (const uint8_t *buf, uint32_t len);
static void Telemetery_BuildPacket(TelemetryPacket_t *pkt);

volatile DHT11_Data_t g_dhtdata_t  = {0};
volatile INA260_Data_t g_inadata_t = {0};
volatile Leak_Data_t g_leakdata_t = {0};

static uint32_t s_seq =  0;




/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  INA260_Init(&hi2c1);
  DWT_Delay_Init();
  LeakSensor_Init(&hadc1);
  App_CreateTaskAndSync();


  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

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
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);
  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
static void DHT11Task(void *argument){

	(void) argument;
	DHT11_Data_t local = {0};
	while(1)
	{

		DHT11_Read(&local);
		osMutexAcquire(g_dhtMutex, osWaitForever);
		g_dhtdata_t.temperature =  local.temperature;
		g_dhtdata_t.humidity = local.humidity;
		g_dhtdata_t.valid =  local.valid;
		osMutexRelease(g_dhtMutex);
		osDelay(pdMS_TO_TICKS(DHT11_PERIOD_MS));

	}


}
static void INA260Task(void * argument){

	(void) argument;
	INA260_Init(&hi2c1);
	INA260_Data_t local =  {0};


	while(1)
	{
		INA260_Read(&local);
		osMutexAcquire(g_inaMutex, osWaitForever);
		g_inadata_t.current_A =  local.current_A;
		g_inadata_t.voltage_V = local.voltage_V;
		g_inadata_t.valid =  local.valid;
		g_inadata_t.errorCount =  local.errorCount;
		osMutexRelease(g_inaMutex);
		osDelay(pdMS_TO_TICKS(INA260_PERIOD_MS));


	}

}
static void LeakTask(void *argument){

	(void)argument;

	LeakSensor_Init(&hadc1);
	Leak_Data_t  local = {0};
	while (1)
	{
		LeakSensor_Read(&local);
		osMutexAcquire(leakTaskHandle, osWaitForever);
		g_leakdata_t.errorCount =  local.errorCount;
		g_leakdata_t.leakDetected = local.leakDetected;
		g_leakdata_t.raw =  local.raw;
		g_leakdata_t.valid = local.valid;
		g_leakdata_t.voltage =  local.voltage;
		osMutexRelease(g_leakMutex);
		osDelay(pdMS_TO_TICKS(LEAK_PERIO_MS));
	}
}
static void UartTask(void *argument){

	(void) argument;
	TelemetryPacket_t pkt;

	 while (1)
	    {
	Telemetery_BuildPacket(&pkt);
	osSemaphoreAcquire(s_txDonesem, osWaitForever);
	HAL_UART_Transmit_DMA(&huart1, (uint8_t *)&pkt, sizeof(pkt));
	osSemaphoreRelease(s_txDonesem);
	osDelay(pdMS_TO_TICKS(UART_PERIOD_MS));

	    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart1)
{
	if (huart1->Instance ==  USART1)
	{
		osSemaphoreRelease(s_txDonesem);

	}
}

static void App_CreateTaskAndSync(void){

	g_dhtMutex =  osMutexNew(NULL);
	g_inaMutex =  osMutexNew(NULL);
	g_leakMutex = osMutexNew(NULL);

	s_txDonesem =  osSemaphoreNew(1, 1, NULL);

	const osThreadAttr_t dhtAttr = {.name = "DHT_TASK", .priority = osPriorityNormal,.stack_size =  256 * 4 };
    const osThreadAttr_t inaAttr  = { .name = "INA260_Task",  .priority = osPriorityNormal,      .stack_size = 256 * 4 };
    const osThreadAttr_t leakAttr  = { .name = "Leak_Task",  .priority = osPriorityNormal,      .stack_size = 256 * 4 };
    const osThreadAttr_t uart_Attr  = { .name = "Uart_task",  .priority = osPriorityAboveNormal,      .stack_size = 256 * 4 };

	dht11TaskHandle = osThreadNew(DHT11Task, NULL, &dhtAttr);
	ina260TaskHandle = osThreadNew(INA260Task, NULL, &inaAttr);
	leakTaskHandle =  osThreadNew(LeakTask, NULL, &leakAttr);
	uartaskHandle =  osThreadNew(UartTask, NULL, &uart_Attr);
}



static uint8_t Telemetery_Checksum (const uint8_t *buf, uint32_t len){

	uint8_t x = 0;
	for (uint8_t i = 0 ; i < len ; i++)
	{
		x ^= buf[i];

	}
	return x;

}
static void Telemetery_BuildPacket(TelemetryPacket_t *pkt){

	DHT11_Data_t dhtsnap;
	INA260_Data_t inasnap;
	Leak_Data_t leaksnap;

	osMutexAcquire(g_dhtMutex, osWaitForever);
	dhtsnap.temperature =  g_dhtdata_t.temperature;
	dhtsnap.humidity =  g_dhtdata_t.humidity;
	dhtsnap.valid =  g_dhtdata_t.valid;
	dhtsnap.errorCount =  g_dhtdata_t.errorCount;
	osMutexRelease(dht11TaskHandle);

	osMutexAcquire(ina260TaskHandle, osWaitForever);
	inasnap.current_A =  g_inadata_t.current_A;
	inasnap.errorCount = g_inadata_t.errorCount;
	inasnap.power_W = g_inadata_t.power_W;
	inasnap.valid =  g_inadata_t.valid;
	inasnap.voltage_V =  g_inadata_t.voltage_V;
	osMutexRelease(ina260TaskHandle);

	osMutexAcquire(leakTaskHandle, osWaitForever);
	leaksnap.errorCount = g_leakdata_t.errorCount;
	leaksnap.leakDetected = g_leakdata_t.leakDetected;
	leaksnap.raw = g_leakdata_t.raw;
	leaksnap.valid =  g_inadata_t.valid;
	leaksnap.voltage =  g_leakdata_t.voltage;
	osMutexRelease(leakTaskHandle);

	pkt->header = TELEMETRY_HEADER_BYTE;
	pkt->temperature =  dhtsnap.temperature;
	pkt->humdity =  dhtsnap.humidity;
	pkt->dhtvalid = dhtsnap.valid;
	pkt->voltage =  inasnap.voltage_V;
	pkt->current =  inasnap.current_A;
	pkt->power =  inasnap.power_W;
	pkt->inaValid = inasnap.valid;
	pkt->leakRaw = leaksnap.raw;
	pkt->leak_Detected =  leaksnap.leakDetected;
	pkt->leakValid =  leaksnap.valid;
	pkt->seqNumber = s_seq++;

	uint32_t coveredLen = offsetof(TelemetryPacket_t, checksum);
	pkt->checksum =  Telemetery_Checksum((const uint8_t *)pkt, coveredLen);
	pkt->footer =  TELEMETERY_FOOTER_BYTE;



}

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
