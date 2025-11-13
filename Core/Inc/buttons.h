#ifndef BUTTONS_H
#define BUTTONS_H

#include "stm32u5xx_hal.h"
#include <stdint.h>

// Button GPIO configuration
#define BTN_LEFT_PORT   GPIOC
#define BTN_LEFT_PIN    GPIO_PIN_0
#define BTN_RIGHT_PORT  GPIOC
#define BTN_RIGHT_PIN   GPIO_PIN_1

// Debounce time in milliseconds
#define DEBOUNCE_TIME_MS    50
#define LONG_PRESS_TIME_MS  1000

typedef struct {
    // Current states (1 = pressed, 0 = released)
    uint8_t left;
    uint8_t right;

    // Edge detection (just pressed this frame)
    uint8_t left_pressed;
    uint8_t right_pressed;

    // Edge detection (just released this frame)
    uint8_t left_released;
    uint8_t right_released;

    // Long press detection
    uint8_t left_long_press;
    uint8_t right_long_press;

    // Both pressed simultaneously
    uint8_t both_pressed;

    // Time tracking for long press
    uint32_t left_press_time;
    uint32_t right_press_time;
} ADCButtonState;

// Button indices
typedef enum {
    BUTTON_LEFT = 0,
    BUTTON_RIGHT = 1,
    BUTTON_COUNT = 2
} ButtonIndex;

// Function prototypes
void Buttons_Init(void);
void Buttons_Update(ADCButtonState* state);
uint8_t Buttons_IsAnyPressed(ADCButtonState* state);
uint8_t Buttons_AreBothPressed(ADCButtonState* state);

#endif // BUTTONS_H
