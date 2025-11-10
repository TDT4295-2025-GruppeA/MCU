#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "main.h"

// Initialize command handler
void Command_Handler_Init(void);

// Process received commands
void Process_UART_Command(const char* cmd);

// Command functions
void Quick_SD_Test(void);
void Show_SD_Info(void);
void Show_Game_Stats(void);
void Test_Shape_Storage(void);
void Run_All_Tests(void);

// UART command buffer
extern char uart_command[64];
extern uint8_t uart_cmd_index;

#endif
