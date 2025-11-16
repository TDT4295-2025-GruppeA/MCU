#include "./Game/input.h"
#include "stm32u5xx_hal.h"
// #include "stm32u5xx_nucleo.h"
#include <string.h>

// External UART for debugging
extern void UART_Printf(const char* format, ...);

// Button state tracking
static ButtonState button_state;
static uint32_t last_button_time = 0;

// Debounce settings
#define DEBOUNCE_TIME_MS    50
#define DOUBLE_CLICK_TIME   500
#define LONG_PRESS_TIME     1000

// Initialize input system
void Input_Init(void)
{
    memset(&button_state, 0, sizeof(ButtonState));
    last_button_time = 0;
}

// Process button input
InputAction Input_ProcessButton(uint8_t button_pressed, uint32_t current_time)
{
    InputAction action = ACTION_NONE;

    // Debounce check
    if(current_time - last_button_time < DEBOUNCE_TIME_MS)
    {
        return ACTION_NONE;
    }

    // Detect press
    if(button_pressed && !button_state.pressed)
    {
        button_state.pressed = 1;
        button_state.press_time = current_time;
        last_button_time = current_time;

        // BSP_LED_On(LED_GREEN);
    }
    // Detect release
    else if(!button_pressed && button_state.pressed)
    {
        button_state.pressed = 0;
        button_state.release_time = current_time;
        uint32_t press_duration = button_state.release_time - button_state.press_time;

        // BSP_LED_Off(LED_GREEN);

        // Check for long press
        if(press_duration > LONG_PRESS_TIME)
        {
            action = ACTION_PAUSE;  // Toggle pause
            UART_Printf("[INPUT] Long press detected\r\n");
        }
        else
        {
            // Short press - check for double click
            button_state.press_count++;

            // Wait for potential second click
            uint32_t wait_start = HAL_GetTick();
            while(HAL_GetTick() - wait_start < DOUBLE_CLICK_TIME)
            {
                // if(BSP_PB_GetState(BUTTON_USER))
                // {
                //     // Wait for release
                //     while(BSP_PB_GetState(BUTTON_USER))
                //     {
                //         HAL_Delay(10);
                //     }
                //     button_state.press_count = 2;
                //     break;
                // }
                HAL_Delay(10);
            }

            // Process click count
            if(button_state.press_count >= 2)
            {
                action = ACTION_MOVE_RIGHT;
                UART_Printf("[INPUT] Double click - Move Right\r\n");
            }
            else
            {
                action = ACTION_MOVE_LEFT;
                UART_Printf("[INPUT] Single click - Move Left\r\n");
            }

            button_state.press_count = 0;
        }

        last_button_time = current_time;
    }

    return action;
}

// Process keyboard input
InputAction Input_ProcessKeyboard(uint8_t key)
{
    InputAction action = ACTION_NONE;

    switch(key)
    {
        case 'a':
        case 'A':
            action = ACTION_MOVE_LEFT;
            UART_Printf("[INPUT] Key A - Move Left\r\n");
            break;

        case 'd':
        case 'D':
            action = ACTION_MOVE_RIGHT;
            UART_Printf("[INPUT] Key D - Move Right\r\n");
            break;

        case 's':
        case 'S':
            action = ACTION_PAUSE;
            UART_Printf("[INPUT] Key S - Pause\r\n");
            break;

        case 'w':
        case 'W':
            action = ACTION_RESUME;
            UART_Printf("[INPUT] Key W - Resume\r\n");
            break;

        case 'r':
        case 'R':
            action = ACTION_RESET;
            UART_Printf("[INPUT] Key R - Reset\r\n");
            break;

        case ' ':
            action = ACTION_JUMP;
            UART_Printf("[INPUT] Space - Jump\r\n");
            break;

        default:
            // Unknown key
            break;
    }

    return action;
}

// Get button state
ButtonState* Input_GetButtonState(void)
{
    return &button_state;
}

// Check if button is currently pressed
uint8_t Input_IsButtonPressed(void)
{
    return button_state.pressed;
}

// Check if button is held for specified time
uint8_t Input_IsButtonHeld(uint32_t hold_time_ms)
{
    if(!button_state.pressed) return 0;

    uint32_t current_time = HAL_GetTick();
    uint32_t press_duration = current_time - button_state.press_time;

    return (press_duration >= hold_time_ms);
}
