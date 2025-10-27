// buttons.c
#include "buttons.h"

// External ADC handle and UART
extern ADC_HandleTypeDef hadc1;
extern void UART_Printf(const char* format, ...);

// Button debounce data
typedef struct {
    uint32_t last_change_time;
    uint32_t press_start_time;
    uint8_t current_state;
    uint8_t previous_state;
    uint8_t raw_state;
    uint8_t long_press_triggered;
} ButtonDebounce;

static ButtonDebounce buttons[BUTTON_COUNT];
static uint8_t buttons_initialized = 0;

// Adds detection for disconnected buttons
static uint8_t buttons_connected = 0;
static uint32_t last_connection_check = 0;


// Check if buttons are actually connected
static uint8_t Check_Buttons_Connected(void)
{
    // Read both channels multiple times
    uint32_t sum_left = 0;
    uint32_t sum_right = 0;

    for(int i = 0; i < 5; i++) {
        sum_left += Read_ADC_Channel(ADC_CHANNEL_11);
        sum_right += Read_ADC_Channel(ADC_CHANNEL_12);
        HAL_Delay(1);
    }

    uint32_t avg_left = sum_left / 5;
    uint32_t avg_right = sum_right / 5;

    // If values are in middle range (1500-2500), pins are likely floating
    // Connected buttons with pull-ups should read near 0 or near 4095
    if ((avg_left > 1500 && avg_left < 2500) ||
        (avg_right > 1500 && avg_right < 2500)) {
        return 0;  // Floating - no buttons connected
    }

    return 1;  // Buttons appear to be connected
}

void Buttons_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Enable clocks
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_ADC12_CLK_ENABLE();

    // Configure PC0 and PC1 as ANALOG inputs for ADC
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // Initialize debounce states
    uint32_t now = HAL_GetTick();
    for(int i = 0; i < BUTTON_COUNT; i++) {
        buttons[i].last_change_time = now;
        buttons[i].press_start_time = 0;
        buttons[i].current_state = 0;
        buttons[i].previous_state = 0;
        buttons[i].raw_state = 0;
        buttons[i].long_press_triggered = 0;
    }

    buttons_initialized = 1;

    // Check if buttons are connected
    buttons_connected = Check_Buttons_Connected();
    if (!buttons_connected) {
        UART_Printf("WARNING: No buttons detected on PC0/PC1 (pins floating)\r\n");
        UART_Printf("         Connect buttons with pull-up resistors to use\r\n");
    } else {
        UART_Printf("Buttons detected on PC0/PC1\r\n");
    }
}

static uint8_t debounce_button_adc(ButtonDebounce* btn, uint32_t adc_channel, uint8_t* long_press) {
    uint32_t now = HAL_GetTick();

    // If buttons not connected, always return not pressed
    if (!buttons_connected) {
        *long_press = 0;
        return 0;
    }

    // Read ADC value
    uint32_t adc_value = Read_ADC_Channel(adc_channel);

    // Convert ADC to button state with better thresholds
    uint8_t raw;
    if(adc_value < 500) {
        raw = 1;  // Button definitely pressed (connected to GND)
    } else if(adc_value > 3500) {
        raw = 0;  // Button definitely released (pulled high)
    } else {
        // In the middle range - keep previous state
        raw = btn->raw_state;
    }

    // If raw state changed, reset debounce timer
    if(raw != btn->raw_state) {
        btn->raw_state = raw;
        btn->last_change_time = now;
    }

    // Update stable state if enough time has passed
    if((now - btn->last_change_time) >= DEBOUNCE_TIME_MS) {
        btn->previous_state = btn->current_state;
        btn->current_state = btn->raw_state;

        // Track press start time
        if(btn->current_state && !btn->previous_state) {
            btn->press_start_time = now;
            btn->long_press_triggered = 0;
        }
        else if(!btn->current_state && btn->previous_state) {
            btn->press_start_time = 0;
            btn->long_press_triggered = 0;
        }
    }

    // Check for long press
    *long_press = 0;
    if(btn->current_state && btn->press_start_time > 0) {
        if((now - btn->press_start_time) >= LONG_PRESS_TIME_MS) {
            if(!btn->long_press_triggered) {
                *long_press = 1;
                btn->long_press_triggered = 1;
            }
        }
    }

    return btn->current_state;
}

void Buttons_Update(ADCButtonState* state) {
    if(!buttons_initialized) {
        Buttons_Init();
    }

    // Periodically check if buttons are connected (every 5 seconds)
    uint32_t now = HAL_GetTick();
    if (now - last_connection_check > 5000) {
        uint8_t prev_connected = buttons_connected;
        buttons_connected = Check_Buttons_Connected();

        if (prev_connected != buttons_connected) {
            if (buttons_connected) {
                UART_Printf("Buttons connected!\r\n");
            } else {
                UART_Printf("Buttons disconnected!\r\n");
            }
        }
        last_connection_check = now;
    }

    // Clear state if buttons not connected
    if (!buttons_connected) {
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
        return;
    }

    uint8_t current[BUTTON_COUNT];
    uint8_t long_press[BUTTON_COUNT];

    // Read buttons via ADC
    current[BUTTON_LEFT] = debounce_button_adc(&buttons[BUTTON_LEFT],
                                                ADC_CHANNEL_11,
                                                &long_press[BUTTON_LEFT]);

    current[BUTTON_RIGHT] = debounce_button_adc(&buttons[BUTTON_RIGHT],
                                                 ADC_CHANNEL_12,
                                                 &long_press[BUTTON_RIGHT]);

    // Update state structure
    state->left = current[BUTTON_LEFT];
    state->right = current[BUTTON_RIGHT];

    state->left_pressed = current[BUTTON_LEFT] && !buttons[BUTTON_LEFT].previous_state;
    state->right_pressed = current[BUTTON_RIGHT] && !buttons[BUTTON_RIGHT].previous_state;

    state->left_released = !current[BUTTON_LEFT] && buttons[BUTTON_LEFT].previous_state;
    state->right_released = !current[BUTTON_RIGHT] && buttons[BUTTON_RIGHT].previous_state;

    state->left_long_press = long_press[BUTTON_LEFT];
    state->right_long_press = long_press[BUTTON_RIGHT];

    state->both_pressed = (state->left_pressed || state->left) &&
                          (state->right_pressed || state->right);

    // Track press duration
    if(buttons[BUTTON_LEFT].press_start_time > 0) {
        state->left_press_time = HAL_GetTick() - buttons[BUTTON_LEFT].press_start_time;
    } else {
        state->left_press_time = 0;
    }

    if(buttons[BUTTON_RIGHT].press_start_time > 0) {
        state->right_press_time = HAL_GetTick() - buttons[BUTTON_RIGHT].press_start_time;
    } else {
        state->right_press_time = 0;
    }
}

uint8_t Buttons_IsAnyPressed(ADCButtonState* state) {
    return state->left || state->right;
}

uint8_t Buttons_AreBothPressed(ADCButtonState* state) {
    return state->left && state->right;
}
