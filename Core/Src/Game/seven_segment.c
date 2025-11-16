#include "stm32u5xx_hal.h"

#include "stm32u5xx_hal.h"
#include "main.h"
#include <stdint.h>
#include <unistd.h> // For HAL_Delay

// Pins for 7-segment display
#define SEG_A_PIN GPIO_PIN_7  // PC7
#define SEG_D_PIN GPIO_PIN_8  // PC8
#define SEG_C_PIN GPIO_PIN_8  // PA8
#define SEG_B_PIN GPIO_PIN_9  // PA9
#define SEG_GPIO_PORT_A GPIOA
#define SEG_GPIO_PORT_C GPIOC

// Display a binary counter (D C B A) on the four pins
void SevenSegment_DisplayBinary(uint8_t value) {
    // Only use lower 4 bits
    // Bit mapping: D (MSB), C, B, A (LSB)
    uint8_t a = value & 0x01;           // A: lowest bit
    uint8_t b = (value >> 1) & 0x01;    // B
    uint8_t c = (value >> 2) & 0x01;    // C
    uint8_t d = (value >> 3) & 0x01;    // D: highest bit

    // Print debug info
    // Example output: Displaying 3: D=0 C=0 B=1 A=1
    printf("Displaying %d: D=%d C=%d B=%d A=%d\n", value, d, c, b, a);

    // Set pins according to D C B A order
    HAL_GPIO_WritePin(SEG_GPIO_PORT_C, SEG_D_PIN, d ? GPIO_PIN_SET : GPIO_PIN_RESET); // D (PC8)
    HAL_GPIO_WritePin(SEG_GPIO_PORT_A, SEG_C_PIN, c ? GPIO_PIN_SET : GPIO_PIN_RESET); // C (PA8)
    HAL_GPIO_WritePin(SEG_GPIO_PORT_A, SEG_B_PIN, b ? GPIO_PIN_SET : GPIO_PIN_RESET); // B (PA9)
    HAL_GPIO_WritePin(SEG_GPIO_PORT_C, SEG_A_PIN, a ? GPIO_PIN_SET : GPIO_PIN_RESET); // A (PC7)
}

void SevenSegment_Loop(void) {
    while(1) {
        for(uint8_t i = 0; i < 10; i++) {
            SevenSegment_DisplayBinary(i);
            HAL_Delay(500); // 500 ms delay
        }
    }
}

void SevenSegment_Init(void) {
    // Enable GPIOA and GPIOC clocks
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // Configure PC7 (A) and PC8 (D) as output
    GPIO_InitTypeDef GPIO_InitStructC = {0};
    GPIO_InitStructC.Pin = SEG_A_PIN | SEG_D_PIN;
    GPIO_InitStructC.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructC.Pull = GPIO_NOPULL;
    GPIO_InitStructC.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SEG_GPIO_PORT_C, &GPIO_InitStructC);

    // Configure PA8 (C) and PA9 (B) as output
    GPIO_InitTypeDef GPIO_InitStructA = {0};
    GPIO_InitStructA.Pin = SEG_C_PIN | SEG_B_PIN;
    GPIO_InitStructA.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructA.Pull = GPIO_NOPULL;
    GPIO_InitStructA.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SEG_GPIO_PORT_A, &GPIO_InitStructA);
}

void SevenSegment_TestPattern(void) {
    // Example: Set A and C ON, B and D OFF
    HAL_GPIO_WritePin(SEG_GPIO_PORT_C, SEG_A_PIN, GPIO_PIN_SET);   // A ON (PC7)
    HAL_GPIO_WritePin(SEG_GPIO_PORT_A, SEG_B_PIN, GPIO_PIN_SET); // B OFF (PA9)
    HAL_GPIO_WritePin(SEG_GPIO_PORT_A, SEG_C_PIN, GPIO_PIN_SET);   // C ON (PA8)
    HAL_GPIO_WritePin(SEG_GPIO_PORT_C, SEG_D_PIN, GPIO_PIN_RESET); // D OFF (PC8)
}
