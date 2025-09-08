/*
 * buttons.c
 *
 *  Created on: Sep 8, 2025
 *      Author: jornik
 */


#include "buttons.h"

// Internal debounce structure
typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
    uint32_t last_change_time;
    uint8_t current_state;
    uint8_t previous_state;
    uint8_t raw_state;
} ButtonDebounce;

// Debounce data for each button
static ButtonDebounce buttons[4];
static uint8_t buttons_initialized = 0;

void Buttons_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Enable GPIO clocks (adjust based on which ports used)
    __HAL_RCC_GPIOC_CLK_ENABLE();
    // If using other ports:
    // __HAL_RCC_GPIOA_CLK_ENABLE();
    // __HAL_RCC_GPIOB_CLK_ENABLE();

    // Configure all button pins as inputs with pull-up
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;  // Buttons connect to GND when pressed
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    // Initialize left button
    GPIO_InitStruct.Pin = BTN_LEFT_PIN;
    HAL_GPIO_Init(BTN_LEFT_PORT, &GPIO_InitStruct);
    buttons[0].port = BTN_LEFT_PORT;
    buttons[0].pin = BTN_LEFT_PIN;

    // Initialize right button
    GPIO_InitStruct.Pin = BTN_RIGHT_PIN;
    HAL_GPIO_Init(BTN_RIGHT_PORT, &GPIO_InitStruct);
    buttons[1].port = BTN_RIGHT_PORT;
    buttons[1].pin = BTN_RIGHT_PIN;

    // Initialize jump button
    GPIO_InitStruct.Pin = BTN_JUMP_PIN;
    HAL_GPIO_Init(BTN_JUMP_PORT, &GPIO_InitStruct);
    buttons[2].port = BTN_JUMP_PORT;
    buttons[2].pin = BTN_JUMP_PIN;

    // Initialize action button
    GPIO_InitStruct.Pin = BTN_ACTION_PIN;
    HAL_GPIO_Init(BTN_ACTION_PORT, &GPIO_InitStruct);
    buttons[3].port = BTN_ACTION_PORT;
    buttons[3].pin = BTN_ACTION_PIN;

    // Initialize debounce states
    uint32_t now = HAL_GetTick();
    for(int i = 0; i < 4; i++) {
        buttons[i].last_change_time = now;
        buttons[i].current_state = 0;
        buttons[i].previous_state = 0;
        buttons[i].raw_state = 0;
    }

    buttons_initialized = 1;
}

static uint8_t debounce_button(ButtonDebounce* btn) {
    uint32_t now = HAL_GetTick();

    // Read raw button state (inverted because pull-up: pressed = 0)
    uint8_t raw = !HAL_GPIO_ReadPin(btn->port, btn->pin);

    // If raw state changed, reset debounce timer
    if(raw != btn->raw_state) {
        btn->raw_state = raw;
        btn->last_change_time = now;
    }

    // Update stable state if enough time has passed
    if((now - btn->last_change_time) >= DEBOUNCE_TIME_MS) {
        btn->previous_state = btn->current_state;
        btn->current_state = btn->raw_state;
    }

    return btn->current_state;
}

void Buttons_Update(ButtonState* state) {
    if(!buttons_initialized) {
        Buttons_Init();
    }

    // Update all button states with debouncing
    uint8_t current[4];
    for(int i = 0; i < 4; i++) {
        current[i] = debounce_button(&buttons[i]);
    }

    // Update current states
    state->left = current[0];
    state->right = current[1];
    state->jump = current[2];
    state->action = current[3];

    // Detect just pressed (rising edge)
    state->left_pressed = current[0] && !buttons[0].previous_state;
    state->right_pressed = current[1] && !buttons[1].previous_state;
    state->jump_pressed = current[2] && !buttons[2].previous_state;
    state->action_pressed = current[3] && !buttons[3].previous_state;

    // Detect just released (falling edge)
    state->left_released = !current[0] && buttons[0].previous_state;
    state->right_released = !current[1] && buttons[1].previous_state;
    state->jump_released = !current[2] && buttons[2].previous_state;
    state->action_released = !current[3] && buttons[3].previous_state;
}

uint8_t Buttons_IsAnyPressed(ButtonState* state) {
    return state->left || state->right || state->jump || state->action;
}
