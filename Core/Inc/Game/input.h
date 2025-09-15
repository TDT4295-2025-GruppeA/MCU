/*
 * input.h
 *
 *  Created on: Sep 15, 2025
 *      Author: jornik
 */

// input.h - Input handling system
#ifndef INPUT_H
#define INPUT_H

#include "game_types.h"
#include <stdint.h>

// Input actions
typedef enum {
    ACTION_NONE = 0,
    ACTION_MOVE_LEFT,
    ACTION_MOVE_RIGHT,
    ACTION_PAUSE,
    ACTION_RESUME,
    ACTION_RESET,
    ACTION_JUMP
} InputAction;

// Initialize input system
void Input_Init(void);

// Process input
InputAction Input_ProcessButton(uint8_t button_state, uint32_t current_time);
InputAction Input_ProcessKeyboard(uint8_t key);

// Get button state
ButtonState* Input_GetButtonState(void);

// Debouncing helpers
uint8_t Input_IsButtonPressed(void);
uint8_t Input_IsButtonHeld(uint32_t hold_time_ms);

#endif // INPUT_H
