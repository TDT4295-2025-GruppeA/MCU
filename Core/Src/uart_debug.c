#include "uart_debug.h"
#include "main.h"
#include "./Test/command_handler.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

extern UART_HandleTypeDef huart1;
static uint8_t uart_rx = 0;


void Debug_Init(void)
{
	HAL_UART_Receive_IT(&huart1, &uart_rx, 1);
}

// UART Printf implementation
void UART_Printf(const char* format, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
}

void Debug_PrintBanner(void)
{
    UART_Printf("\r\n=================================\r\n");
    UART_Printf("System Initialized\r\n");
    UART_Printf("SPI1: FPGA Communication\r\n");
    UART_Printf("SPI3: SD Card Communication\r\n");
    UART_Printf("ADC1: Calibrated and ready\r\n");
    UART_Printf("=================================\r\n");
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)
    {
        HAL_UART_Receive_IT(&huart1, &uart_rx, 1);
    }
}
