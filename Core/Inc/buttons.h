/*
 * buttons.h
 *
 *  Created on: Sep 8, 2025
 *      Author: jornik
 */

#ifndef BUTTONS_H
#define BUTTONS_H

#include "main.h"

// Button pins
#define BTN_LEFT_PIN   GPIO_PIN_0
#define BTN_LEFT_PORT  GPIOC
#define BTN_RIGHT_PIN  GPIO_PIN_1
#define BTN_RIGHT_PORT GPIOC

// add more buttons here
#define BTN_JUMP_PIN   GPIO_PIN_2
#define BTN_JUMP_PORT  GPIOC
#define BTN_ACTION_PIN GPIO_PIN_3
#define BTN_ACTION_PORT GPIOC

// Debounce time in milliseconds
#define DEBOUNCE_TIME_MS 30

// Button state structure
typedef struct {
    // Current state
    uint8_t left;
    uint8_t right;
    uint8_t jump;
    uint8_t action;

    // Edge detection (just pressed this frame)
    uint8_t left_pressed;
    uint8_t right_pressed;
    uint8_t jump_pressed;
    uint8_t action_pressed;

    // Edge detection (just released this frame)
    uint8_t left_released;
    uint8_t right_released;
    uint8_t jump_released;
    uint8_t action_released;
} ButtonState;

// Function prototypes
void Buttons_Init(void);
void Buttons_Update(ButtonState* state);
uint8_t Buttons_IsAnyPressed(ButtonState* state);

#endif
