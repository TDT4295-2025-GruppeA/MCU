#include "main.h"
#include "./Game/game.h"
#include "./Game/seven_segment.h"
#include "./Test/command_handler.h"
#include "buttons.h"
#include "peripherals.h"
#include "uart_debug.h"
#include "adc_functions.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>


#ifdef RUN_UNIT_TESTS
    #include "./SDCard/sd_card.h"
    #include "./SDCard/game_storage.h"
    #include "./Game/shapes.h"

    // Test functions
    extern void Run_Obstacle_Tests(void);
    extern void Run_SDCard_Tests(void);
    extern void Run_Collision_Tests(void);
#endif

// COM_InitTypeDef BspCOMInit;
ADC_HandleTypeDef hadc1;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi3;

// UART_HandleTypeDef huart1;
uint8_t uart_rx = 0;

void SystemClock_Config(void);
static void SystemPower_Config(void);

void UART_Printf(const char* format, ...) {
  // do noithing;
}

uint32_t Read_ADC_Channel(uint32_t channel);

#ifdef RUN_UNIT_TESTS
// Only compile this function when in test mode
void Run_All_Unit_Tests(void) {
    UART_Printf("\r\n=== RUNNING UNIT TESTS ===\r\n");

    // Initialize what we need for tests
    if(Storage_Init(&hspi3) == SD_OK) {
        UART_Printf("SD Card ready for testing\r\n");
        Shapes_Init();
        Storage_InitializeShapes();
    }

    // Run each test suite
    Run_Collision_Tests();
    Run_Obstacle_Tests();

    if(SD_IsPresent()) {
        Run_SDCard_Tests();
    } else {
        UART_Printf("Skipping SD tests - no card\r\n");
    }

    UART_Printf("\r\n=== ALL TESTS COMPLETE ===\r\n");
}
#endif


/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  HAL_Init();
  SystemPower_Config();
  SystemClock_Config();

  /* Initialize all configured peripherals */
  Peripherals_Init();
  // Debug_Init();

  // BSP_LED_Init(LED_GREEN);

  // Configure COM port for BSP
  // BspCOMInit.BaudRate   = 115200;
  // BspCOMInit.WordLength = COM_WORDLENGTH_8B;
  // BspCOMInit.StopBits   = COM_STOPBITS_1;
  // BspCOMInit.Parity     = COM_PARITY_NONE;
  // BspCOMInit.HwFlowCtl  = COM_HWCONTROL_NONE;
  // BSP_COM_Init(COM1, &BspCOMInit);

  // Start UART interrupt
  // HAL_UART_Receive_IT(&huart1, &uart_rx, 1);

  // Run ADC calibration
  if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK)
  {
      Error_Handler();
  }

  #ifdef RUN_UNIT_TESTS
	Run_All_Unit_Tests();
	while(1) { HAL_Delay(1000); }
	#else
		Game_Init();
  #endif

  // if (BSP_COM_Init(COM1, &BspCOMInit) != BSP_ERROR_NONE)
  // {
  //   Error_Handler();
  // }

  while (1)
  {
    uint32_t now = HAL_GetTick();
    /* Run the full game update loop (handles buttons, movement, rendering) */
    // SevenSegment_Init();
    // SevenSegment_Loop();
    Game_Update(now);
  }
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE4) != HAL_OK)
  {
      Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_4;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Power Configuration
  * @retval None
  */
static void SystemPower_Config(void)
{

  /*
   * Switch to SMPS regulator instead of LDO
   */
  if (HAL_PWREx_ConfigSupply(PWR_SMPS_SUPPLY) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
      // BSP_LED_Toggle(LED_GREEN);
      HAL_Delay(100);
  }
}
