/*
 * game.c
 *
 *  Created on: Sep 12, 2025
 *      Author: jornik
 */


// game.c - Game Logic Implementation
#include "game.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

// Global game state
Position pos = {0, 0, 0};
ButtonState btn = {0, 0, 0, 0};
uint8_t moving_forward = 1;
uint32_t frame_count = 0;
static uint32_t last_update = 0;

// External handles (from main.c)
extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;

// External UART function
extern void UART_Printf(const char* format, ...);

// Initialize game
void Game_Init(void)
{
    pos.x = 0;
    pos.y = 0;
    pos.z = 0;
    moving_forward = 1;
    frame_count = 0;
    last_update = HAL_GetTick();

    UART_Printf("\r\n=================================\r\n");
    UART_Printf("SIMPLE POSITION TRACKER\r\n");
    UART_Printf("=================================\r\n");
    UART_Printf("Controls:\r\n");
    UART_Printf("  Button: 1 click = LEFT\r\n");
    UART_Printf("         2 clicks = RIGHT\r\n");
    UART_Printf("         Hold = STOP/START\r\n");
    UART_Printf("  Keys: A/D = left/right\r\n");
    UART_Printf("        S = stop, W = go\r\n");
    UART_Printf("        R = reset\r\n");
    UART_Printf("\r\nStarting...\r\n\r\n");
}

// Main game update
void Game_Update(uint32_t current_time)
{
    // Check if it's time for position update
    if(current_time - last_update >= UPDATE_INTERVAL)
    {
        last_update = current_time;
        frame_count++;

        // Move forward if enabled
        if(moving_forward)
        {
            pos.z += FORWARD_SPEED * (UPDATE_INTERVAL / 1000.0f);
        }

        // Keep X in bounds
        if(pos.x < WORLD_MIN_X) pos.x = WORLD_MIN_X;
        if(pos.x > WORLD_MAX_X) pos.x = WORLD_MAX_X;

        // Print position - USE INTEGER FORMAT!
        UART_Printf("\r\n[%lu] POS: X=%d, Y=%d, Z=%d %s\r\n",
                   frame_count,
                   (int)pos.x,    // Cast to int for printing
                   (int)pos.y,
                   (int)pos.z,
                   moving_forward ? "" : "(STOPPED)");

        // Send to FPGA
        Game_SendPositionToFPGA();
    }
}

// Handle button input
void Game_HandleButton(uint8_t button_state, uint32_t current_time)
{
    // Detect press
    if(button_state && !btn.pressed)
    {
        btn.pressed = 1;
        btn.press_time = current_time;
        BSP_LED_On(LED_GREEN);
    }

    // Detect release
    if(!button_state && btn.pressed)
    {
        btn.pressed = 0;
        btn.release_time = current_time;
        uint32_t duration = btn.release_time - btn.press_time;
        BSP_LED_Off(LED_GREEN);

        if(duration > 1000)
        {
            // Long press = toggle movement
            moving_forward = !moving_forward;
            UART_Printf("[BUTTON] %s\r\n", moving_forward ? "START" : "STOP");
        }
        else
        {
            // Short press - count it
            btn.press_count++;

            // Wait for potential second click
            uint32_t wait_start = HAL_GetTick();
            while(HAL_GetTick() - wait_start < 500)
            {
                if(BSP_PB_GetState(BUTTON_USER))
                {
                    // Second click detected
                    btn.press_count = 2;
                    break;
                }
                HAL_Delay(10);
            }

            // Process clicks
            if(btn.press_count == 2)
            {
                // Double click = RIGHT
                pos.x += STRAFE_SPEED;
                UART_Printf("[BUTTON] RIGHT -> X=%d\r\n", (int)pos.x);  // USE INT!
            }
            else
            {
                // Single click = LEFT
                pos.x -= STRAFE_SPEED;
                UART_Printf("[BUTTON] LEFT -> X=%d\r\n", (int)pos.x);   // USE INT!
            }

            btn.press_count = 0;
        }
    }
}

// Handle keyboard input
void Game_HandleKeyboard(uint8_t key)
{
    switch(key)
    {
        case 'a':
        case 'A':
            pos.x -= STRAFE_SPEED;
            UART_Printf("[KEY] LEFT -> X=%d\r\n", (int)pos.x);  // USE INT!
            break;

        case 'd':
        case 'D':
            pos.x += STRAFE_SPEED;
            UART_Printf("[KEY] RIGHT -> X=%d\r\n", (int)pos.x); // USE INT!
            break;

        case 's':
        case 'S':
            moving_forward = 0;
            UART_Printf("[KEY] STOP\r\n");
            break;

        case 'w':
        case 'W':
            moving_forward = 1;
            UART_Printf("[KEY] GO\r\n");
            break;

        case 'r':
        case 'R':
            pos.x = 0;
            pos.y = 0;
            pos.z = 0;
            UART_Printf("[KEY] RESET\r\n");
            break;
    }
}

// Send position to FPGA via SPI
void Game_SendPositionToFPGA(void)
{
    // Packet structure for FPGA
    struct {
        uint8_t cmd;      // 0x01 = POSITION
        int16_t x;
        int16_t y;
        int16_t z;
    } __attribute__((packed)) packet;

    packet.cmd = 0x01;
    packet.x = (int16_t)pos.x;
    packet.y = (int16_t)pos.y;
    packet.z = (int16_t)pos.z;

    // Send via SPI
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);  // CS Low
    HAL_SPI_Transmit(&hspi1, (uint8_t*)&packet, sizeof(packet), 10);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);    // CS High

    // Show SPI data sent (hex values are fine)
    UART_Printf("SPI->[%02X %04X %04X %04X]\r\n",
                packet.cmd, packet.x, packet.y, packet.z);
}
