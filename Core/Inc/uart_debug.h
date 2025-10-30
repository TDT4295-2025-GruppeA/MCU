
#ifndef INC_UART_DEBUG_H_
#define INC_UART_DEBUG_H_
#include "stm32u5xx_hal.h"

void UART_Printf(const char* format, ...);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void Debug_PrintBanner(void);
void Debug_Init(void);

#endif /* INC_UART_DEBUG_H_ */
