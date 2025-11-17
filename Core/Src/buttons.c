
#include "buttons.h"
#include "adc_functions.h"

// External UART
extern void UART_Printf(const char* format, ...);

// Zone thresholds
#define POT_LEFT_THRESHOLD  (POT_CENTER - POT_DEADZONE)  // ~6992
#define POT_RIGHT_THRESHOLD (POT_CENTER + POT_DEADZONE)  // ~9392

// Tracking state
static uint32_t last_pot_value = POT_CENTER;
static uint8_t current_zone = 0;  // 0=center, 1=left, 2=right
static uint8_t previous_zone = 0;
static uint32_t zone_enter_time = 0;
static uint8_t buttons_initialized = 0;
static uint32_t debug_counter = 0;
// Store last raw ADC value for analog input
static uint32_t last_adc_value = POT_CENTER;

void Buttons_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Enable clocks
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_ADC12_CLK_ENABLE();

    // Configure PC3 as ANALOG input for ADC
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
    
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);

    buttons_initialized = 1;

    // Read initial position - USE CHANNEL 1!
    last_pot_value = Read_ADC_Channel(ADC_CHANNEL_4);

    UART_Printf("Potentiometer initialized on PC3 (14-bit ADC)\r\n");
    UART_Printf("  Initial reading: %lu\r\n", last_pot_value);
    UART_Printf("  Left zone: < %d\r\n", POT_LEFT_THRESHOLD);
    UART_Printf("  Dead zone: %d - %d\r\n", POT_LEFT_THRESHOLD, POT_RIGHT_THRESHOLD);
    UART_Printf("  Right zone: > %d\r\n", POT_RIGHT_THRESHOLD);
}

void Buttons_Update(ADCButtonState* state) {
    if(!buttons_initialized) {
        Buttons_Init();
    }

    uint32_t now = HAL_GetTick();

    // Read potentiometer value
    uint32_t pot_value = Read_ADC_Channel(ADC_CHANNEL_4);
    last_adc_value = pot_value;

    // Debug output every 40 calls
    debug_counter++;
    if(debug_counter >= 40) {
        debug_counter = 0;
    }

    // Determine current zone
    previous_zone = current_zone;

    if(pot_value < POT_LEFT_THRESHOLD) {
        current_zone = 1;  // Left
    } else if(pot_value > POT_RIGHT_THRESHOLD) {
        current_zone = 2;  // Right
    } else {
        current_zone = 0;  // Center/neutral
    }

    // Track when we entered a new zone
    if(current_zone != previous_zone) {
        zone_enter_time = now;

        UART_Printf("Zone changed: %s (value: %lu)\r\n",
                   current_zone == 1 ? "LEFT" :
                   current_zone == 2 ? "RIGHT" : "CENTER",
                   pot_value);
    }

    // Clear all states first
    state->left = 0;
    state->right = 0;
    state->left_pressed = 0;
    state->right_pressed = 0;
    state->left_released = 0;
    state->right_released = 0;
    state->left_long_press = 0;
    state->right_long_press = 0;
    state->both_pressed = 0;
    state->left_press_time = 0;
    state->right_press_time = 0;

    // Set current state based on zone
    state->left = (current_zone == 1);
    state->right = (current_zone == 2);

    // Detect transitions
    state->left_pressed = (current_zone == 1 && previous_zone != 1);
    state->right_pressed = (current_zone == 2 && previous_zone != 2);
    state->left_released = (previous_zone == 1 && current_zone != 1);
    state->right_released = (previous_zone == 2 && current_zone != 2);

    // Track how long we've been in a zone (for long press)
    if(current_zone != 0 && zone_enter_time > 0) {
        uint32_t time_in_zone = now - zone_enter_time;

        if(current_zone == 1) {
            state->left_press_time = time_in_zone;
            if(time_in_zone >= LONG_PRESS_TIME_MS) {
                state->left_long_press = 1;
            }
        } else if(current_zone == 2) {
            state->right_press_time = time_in_zone;
            if(time_in_zone >= LONG_PRESS_TIME_MS) {
                state->right_long_press = 1;
            }
        }
    }

    last_pot_value = pot_value;
}

uint8_t Buttons_IsAnyPressed(ADCButtonState* state) {
    return state->left || state->right;
}

uint8_t Buttons_AreBothPressed(ADCButtonState* state) {
    return 0;  // Can't press both with single pot
}

// Getter for last ADC value
uint32_t Buttons_GetLastADCValue(void) {
    return last_adc_value;
}
