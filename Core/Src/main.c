
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "string.h"
#include "stdio.h"
#include "stdarg.h"


SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart1;

COM_InitTypeDef BspCOMInit;

SPI_HandleTypeDef hspi1;



/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void SystemPower_Config(void);
static void MX_GPIO_Init(void);
static void MX_ICACHE_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);

// Test functions
void Test_SPI_Loopback(void);
void Test_SPI_Basic(void);
void Test_FPGA_Commands(void);
void UART_Printf(const char* format, ...);

/* Global test state */
uint8_t test_mode = 0;
uint8_t button_press_count = 0;

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

  /* Configure the System Power */
  SystemPower_Config();

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ICACHE_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Initialize led */
  BSP_LED_Init(LED_GREEN);

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Initialize COM1 port (115200, 8 bits (7-bit data + 1 stop bit), no parity */
  BspCOMInit.BaudRate   = 115200;
  BspCOMInit.WordLength = COM_WORDLENGTH_8B;
  BspCOMInit.StopBits   = COM_STOPBITS_1;
  BspCOMInit.Parity     = COM_PARITY_NONE;
  BspCOMInit.HwFlowCtl  = COM_HWCONTROL_NONE;
  if (BSP_COM_Init(COM1, &BspCOMInit) != BSP_ERROR_NONE)
  {
    Error_Handler();
  }

  /* Send startup message */
  UART_Printf("\r\n==================================\r\n");
  UART_Printf("STM32U5 SPI Test Program Started!\r\n");
  UART_Printf("==================================\r\n\r\n");
  UART_Printf("Press USER button to cycle through tests:\r\n");
  UART_Printf("1. SPI Loopback Test (connect MOSI to MISO)\r\n");
  UART_Printf("2. SPI Basic Transmission Test\r\n");
  UART_Printf("3. FPGA Command Test\r\n\r\n");

  uint32_t last_test_time = 0;

  while (1)
     {
         uint32_t current_time = HAL_GetTick();

         // Run test every 2 seconds if in test mode
         if(test_mode > 0 && (current_time - last_test_time) >= 2000)
         {
             last_test_time = current_time;

             switch(test_mode)
             {
                 case 1:
                     Test_SPI_Loopback();
                     break;
                 case 2:
                     Test_SPI_Basic();
                     break;
                 case 3:
                     Test_FPGA_Commands();
                     break;
             }
         }

         HAL_Delay(10);
     }
}

/* Button callback - cycles through test modes */
void BSP_PB_Callback(Button_TypeDef Button)
{
    if (Button == BUTTON_USER)
    {
        button_press_count++;
        test_mode = (test_mode + 1) % 4;  // Cycle through 0,1,2,3

        BSP_LED_Toggle(LED_GREEN);

        switch(test_mode)
        {
            case 0:
                UART_Printf("\n[BUTTON] Test Mode: OFF\r\n");
                BSP_LED_Off(LED_GREEN);
                break;
            case 1:
                UART_Printf("\n[BUTTON] Test Mode: SPI LOOPBACK\r\n");
                UART_Printf("Connect MOSI (PA7) to MISO (PA6) for loopback test\r\n");
                break;
            case 2:
                UART_Printf("\n[BUTTON] Test Mode: SPI BASIC\r\n");
                break;
            case 3:
                UART_Printf("\n[BUTTON] Test Mode: FPGA COMMANDS\r\n");
                break;
        }
    }
}

/* Test 1: SPI Loopback Test */
void Test_SPI_Loopback(void)
{
    UART_Printf("\n--- SPI Loopback Test ---\r\n");

    uint8_t tx_data[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    uint8_t rx_data[8] = {0};

    // For loopback test, we need to use TransmitReceive
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&hspi1, tx_data, rx_data, 8, 100);

    if(status == HAL_OK)
    {
        UART_Printf("TX: ");
        for(int i = 0; i < 8; i++)
            UART_Printf("%02X ", tx_data[i]);
        UART_Printf("\r\n");

        UART_Printf("RX: ");
        for(int i = 0; i < 8; i++)
            UART_Printf("%02X ", rx_data[i]);
        UART_Printf("\r\n");

        // Check if data matches
        uint8_t match = 1;
        for(int i = 0; i < 8; i++)
        {
            if(tx_data[i] != rx_data[i])
            {
                match = 0;
                break;
            }
        }

        if(match)
        {
            UART_Printf("SUCCESS: Data matches! SPI is working.\r\n");
            BSP_LED_On(LED_GREEN);
            HAL_Delay(100);
            BSP_LED_Off(LED_GREEN);
        }
        else
        {
            UART_Printf("FAIL: Data mismatch. Check MOSI-MISO connection.\r\n");
        }
    }
    else
    {
        UART_Printf("ERROR: SPI TransmitReceive failed (status=%d)\r\n", status);
    }
}

/* Test 2: Basic SPI Transmission (monitor with oscilloscope/logic analyzer) */
void Test_SPI_Basic(void)
{
    UART_Printf("\n--- SPI Basic Transmission Test ---\r\n");

    // Test pattern that's easy to see on scope
    uint8_t test_pattern[] = {0xAA, 0x55, 0xF0, 0x0F, 0xFF, 0x00};

    UART_Printf("Sending: ");
    for(int i = 0; i < 6; i++)
        UART_Printf("%02X ", test_pattern[i]);
    UART_Printf("\r\n");

    // Manual CS control for testing
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);  // CS Low
    HAL_Delay(1);

    HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, test_pattern, 6, 100);

    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);    // CS High

    if(status == HAL_OK)
    {
        UART_Printf("Transmission complete! Check with scope:\r\n");
        UART_Printf("  - SCK (PA5): Should show clock pulses\r\n");
        UART_Printf("  - MOSI (PA7): Should show data pattern\r\n");
        UART_Printf("  - CS (PA4): Should pulse low\r\n");
    }
    else
    {
        UART_Printf("Transmission failed (status=%d)\r\n", status);
    }
}

/* Test 3: FPGA-style Commands */
void Test_FPGA_Commands(void)
{
    UART_Printf("\n--- FPGA Command Test ---\r\n");

    // Simulate FPGA protocol commands
    typedef struct {
        uint8_t command;
        uint8_t x_low;
        uint8_t x_high;
        uint8_t y_low;
        uint8_t y_high;
        uint8_t r;
        uint8_t g;
        uint8_t b;
    } DrawPixelCommand;

    DrawPixelCommand cmd = {
        .command = 0x02,  // DRAW_PIXEL command
        .x_low = 100 & 0xFF,
        .x_high = (100 >> 8) & 0xFF,
        .y_low = 200 & 0xFF,
        .y_high = (200 >> 8) & 0xFF,
        .r = 255,
        .g = 0,
        .b = 128
    };

    UART_Printf("Sending DRAW_PIXEL command:\r\n");
    UART_Printf("  Cmd=0x%02X, X=%d, Y=%d, RGB=(%d,%d,%d)\r\n",
                cmd.command, 100, 200, cmd.r, cmd.g, cmd.b);

    // Send command with CS control
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);  // CS Low

    HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, (uint8_t*)&cmd, sizeof(cmd), 100);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);    // CS High

    if(status == HAL_OK)
    {
        UART_Printf("Command sent successfully!\r\n");

        // Flash LED to indicate transmission
        for(int i = 0; i < 3; i++)
        {
            BSP_LED_Toggle(LED_GREEN);
            HAL_Delay(50);
        }
        BSP_LED_Off(LED_GREEN);
    }
    else
    {
        UART_Printf("Command failed (status=%d)\r\n", status);
    }

    // Test different commands
    static uint8_t cmd_counter = 0;
    cmd_counter++;

    if(cmd_counter % 3 == 1)
    {
        // Clear screen command
        uint8_t clear_cmd[] = {0x01, 0x00, 0x00, 0x00};  // CMD_CLEAR_SCREEN
        UART_Printf("Sending CLEAR_SCREEN command\r\n");

        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
        HAL_SPI_Transmit(&hspi1, clear_cmd, 4, 100);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    }
}


static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* UART Printf implementation */
void UART_Printf(const char* format, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    static uint8_t rx_byte;

    if(huart->Instance == USART1)
    {
        // Process received command
        switch(rx_byte)
        {
            case '1':
                test_mode = 1;
                UART_Printf("\nSwitched to LOOPBACK test\r\n");
                break;
            case '2':
                test_mode = 2;
                UART_Printf("\nSwitched to BASIC test\r\n");
                break;
            case '3':
                test_mode = 3;
                UART_Printf("\nSwitched to FPGA test\r\n");
                break;
            case '0':
                test_mode = 0;
                UART_Printf("\nTests stopped\r\n");
                break;
            case 't':
                SPI_AnalyzeTiming(&hspi1);
                break;
            case 's':
                SPI_SpeedTest(&hspi1);
                break;
            case '\r':
            case '\n':
                // Run current test immediately
                if(test_mode == 1) Test_SPI_Loopback();
                else if(test_mode == 2) Test_SPI_Basic();
                else if(test_mode == 3) Test_FPGA_Commands();
                break;
        }

        // Re-enable reception
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
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
/* USER CODE BEGIN PWR */
/* USER CODE END PWR */
}

/**
  * @brief ICACHE Initialization Function
  * @param None
  * @retval None
  */
static void MX_ICACHE_Init(void)
{

  /* USER CODE BEGIN ICACHE_Init 0 */

  /* USER CODE END ICACHE_Init 0 */

  /* USER CODE BEGIN ICACHE_Init 1 */

  /* USER CODE END ICACHE_Init 1 */

  /** Enable instruction cache in 1-way (direct mapped cache)
  */
  if (HAL_ICACHE_ConfigAssociativityMode(ICACHE_1WAY) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_ICACHE_Enable() != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ICACHE_Init 2 */

  /* USER CODE END ICACHE_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  SPI_AutonomousModeConfTypeDef HAL_SPI_AutonomousMode_Cfg_Struct = {0};

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 0x7;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi1.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi1.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi1.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi1.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi1.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi1.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  hspi1.Init.ReadyMasterManagement = SPI_RDY_MASTER_MANAGEMENT_INTERNALLY;
  hspi1.Init.ReadyPolarity = SPI_RDY_POLARITY_HIGH;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerState = SPI_AUTO_MODE_DISABLE;
  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerSelection = SPI_GRP1_GPDMA_CH0_TCF_TRG;
  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerPolarity = SPI_TRIG_POLARITY_RISING;
  if (HAL_SPIEx_SetConfigAutonomousMode(&hspi1, &HAL_SPI_AutonomousMode_Cfg_Struct) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

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
#ifdef USE_FULL_ASSERT
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
