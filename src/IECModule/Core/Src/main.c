/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "modbusSlave.h"
#include "IECData.h"
#include "config_params.h"
#include "stm32g0xx_hal.h"
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
// Az utolsó, 30-es számú lap kezdőcíme (64KB-os G030 esetén)

#define SETTINGS_FLASH_ADDR 0x0800E000

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

uint8_t RxData[256];
uint8_t TxData[256];

uint8_t MODBUS_ID = 1;

uint8_t prevRelayState;
uint8_t prevCustCurrWarningLimit;
uint8_t prevCustCurrErrorLimit;

float actualCurrent;

typedef struct {
    int   meas_avg_num;
    float custcurr_warning_limit;
    float custcurr_error_limit;
    float overcurrent_treshold;
} AppSettings_t;

AppSettings_t IECSettings;
uint8_t UpdateDataReceived = 0;

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if (RxData[0] == Config_Params->MODBUS_ID)
	{
		switch (RxData[1]){
		case 0x03:
			readHoldingRegs();
			break;
		case 0x04:
			readInputRegs();
			break;
		case 0x01:
			readCoils();
			break;
		case 0x02:
			readInputs();
			break;
		case 0x06:
			writeSingleReg();
			break;
		case 0x10:
			writeHoldingRegs();
			UpdateDataReceived = 1;
			break;
		case 0x05:
			writeSingleCoil();
			break;
		case 0x0F:
			writeMultiCoils();
			break;
		default:
			modbusException(ILLEGAL_FUNCTION);
			break;
		}
	}
	HAL_UARTEx_ReceiveToIdle_IT(&huart1, RxData, 256);
}

HAL_StatusTypeDef Flash_SaveSettings(AppSettings_t *settings)
{
    HAL_StatusTypeDef status;

    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase;
    uint32_t pageError;

    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.Page      = (SETTINGS_FLASH_ADDR - FLASH_BASE) / FLASH_PAGE_SIZE;
    erase.NbPages   = 1;

    status = HAL_FLASHEx_Erase(&erase, &pageError);
    if(status != HAL_OK)
    {
        HAL_FLASH_Lock();
        return status;
    }

    uint64_t *data = (uint64_t*)settings;
    uint32_t address = SETTINGS_FLASH_ADDR;

    for(int i = 0; i < sizeof(AppSettings_t)/8; i++)
    {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, data[i]);
        if(status != HAL_OK)
        {
            HAL_FLASH_Lock();
            return status;
        }

        address += 8;
    }

    HAL_FLASH_Lock();

    return HAL_OK;
}

void Flash_LoadSettings(AppSettings_t *settings)
{
    memcpy(settings, (void*)SETTINGS_FLASH_ADDR, sizeof(AppSettings_t));
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
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_UARTEx_ReceiveToIdle_IT(&huart1, RxData, 256);
  HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);
  prevRelayState = 0;
  HAL_GPIO_WritePin(RELAY_CTRL_GPIO_Port, RELAY_CTRL_Pin, prevRelayState);
  Coils_Database[RELAY_STATE_ADDR] = prevRelayState;

  Coils_Database[IS_RMS_VOLTAGE_MEASURED_ADDR] = Config_Params->IS_RMS_VOLTAGE_MEASURED;
  Coils_Database[IS_RMS_CURRENT_MEASURED_ADDR] = Config_Params->IS_RMS_CURRENT_MEASURED;

  Coils_Database[IS_ACTIVE_POWER_MEASURED_ADDR] = Config_Params->IS_ACTIVE_POWER_MEASURED;
  Coils_Database[IS_REACTIVE_POWER_MEASURED_ADDR] = Config_Params->IS_REACTIVE_POWER_MEASURED;
  Coils_Database[IS_APPARENT_POWER_MEASURED_ADDR] = Config_Params->IS_APPARENT_POWER_MEASURED;
  Coils_Database[IS_POWER_FACTOR_MEASURED_ADDR] = Config_Params->IS_POWER_FACTOR_MEASURED;
  Coils_Database[IS_AC_FREQUENCY_MEASURED_ADDR] = Config_Params->IS_AC_FREQUENCY_MEASURED;

  Input_Registers_Database[ID_ADDR] = Config_Params->ID;
  Input_Registers_Database[VERSION_ADDR] = Config_Params->VERSION;
  Input_Registers_Database[AVAILABLE_LEDS_ADDR] = Config_Params->AVAILABLE_LEDS;

  Input_Registers_Database[CURRENT_LIMIT_ADDR] = Config_Params->CURRENT_LIMIT;
  Input_Registers_Database[RELAY_COUNT_ADDR] = Config_Params->RELAY_COUNT;
  MODBUS_ID = Config_Params->MODBUS_ID;

	// Float-ként inicializáljuk az alapértékeket!
   addFloatToRegister(Holding_Registers_Database, MEAS_AVG_NUM_ADDR, 10.0);
   addFloatToRegister(Holding_Registers_Database, CUSTCURR_WARNING_LIMIT_ADDR, (float)Config_Params->CURRENT_LIMIT);
   addFloatToRegister(Holding_Registers_Database, CUSTCURR_ERROR_LIMIT_ADDR, (float)Config_Params->CURRENT_LIMIT);
   addFloatToRegister(Holding_Registers_Database, OC_TRESHOLD_ADDR, (float)Config_Params->CURRENT_LIMIT);

   Flash_LoadSettings(&IECSettings);

   // 2. Ellenőrizzük, hogy üres-e a Flash.
   if (IECSettings.meas_avg_num == 0xFFFFFFFF) {
	   float currLim = (float)Config_Params->CURRENT_LIMIT;
	   IECSettings.meas_avg_num = 10.0;
	   IECSettings.custcurr_warning_limit = currLim;
	   IECSettings.custcurr_error_limit = currLim;
	   IECSettings.overcurrent_treshold = currLim;
	   Flash_SaveSettings(&IECSettings);
   }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if (UpdateDataReceived) {
				  IECSettings.meas_avg_num = Holding_Registers_Database[MEAS_AVG_NUM_ADDR];
				  IECSettings.custcurr_warning_limit = Holding_Registers_Database[CUSTCURR_WARNING_LIMIT_ADDR];
				  IECSettings.custcurr_error_limit = Holding_Registers_Database[CUSTCURR_ERROR_LIMIT_ADDR];
				  IECSettings.overcurrent_treshold = Holding_Registers_Database[OC_TRESHOLD_ADDR];
				  //Flash_SaveSettings(&IECSettings);
	             UpdateDataReceived = 0;
	         }
		readCurrentData(&hadc1);
		readVoltageData();
		readPowerData(&hadc1);
		prevRelayState = setRelayStatus(prevRelayState);
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

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
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

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.LowPowerAutoPowerOff = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.SamplingTimeCommon1 = ADC_SAMPLETIME_1CYCLE_5;
  hadc1.Init.SamplingTimeCommon2 = ADC_SAMPLETIME_1CYCLE_5;
  hadc1.Init.OversamplingMode = DISABLE;
  hadc1.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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
  huart1.Init.BaudRate = 19200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GREEN_LED_Pin|RED_LED_Pin|MCU_DERE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(RELAY_CTRL_GPIO_Port, RELAY_CTRL_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : GREEN_LED_Pin RED_LED_Pin MCU_DERE_Pin */
  GPIO_InitStruct.Pin = GREEN_LED_Pin|RED_LED_Pin|MCU_DERE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : RELAY_CTRL_Pin */
  GPIO_InitStruct.Pin = RELAY_CTRL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(RELAY_CTRL_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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
