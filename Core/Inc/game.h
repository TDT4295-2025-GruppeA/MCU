/*
 * game.h
 *
 *  Created on: Sep 12, 2025
 *      Author: jornik
 */

// game.h - Game Logic Header
#ifndef GAME_H
#define GAME_H

#include "main.h"

// Position structure
typedef struct {
    float x;      // Left/Right (-100 to +100)
    float y;      // Up/Down (stays at 0 for now)
    float z;      // Forward (always increasing)
} Position;

// Button control
typedef struct {
    uint8_t pressed;
    uint32_t press_time;
    uint32_t release_time;
    uint8_t press_count;
} ButtonState;

// Game configuration
#define FORWARD_SPEED   2.0f     // Units per second
#define STRAFE_SPEED    20.0f    // Units per movement
#define UPDATE_INTERVAL 1500     // ms between updates
#define WORLD_MIN_X     -100.0f
#define WORLD_MAX_X     100.0f

// Global game state (extern declarations)
extern Position pos;
extern ButtonState btn;
extern uint8_t moving_forward;
extern uint32_t frame_count;

// Function prototypes
void Game_Init(void);
void Game_Update(uint32_t current_time);
void Game_HandleButton(uint8_t button_state, uint32_t current_time);
void Game_HandleKeyboard(uint8_t key);
void Game_SendPositionToFPGA(void);

#endif // GAME_H
