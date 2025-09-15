/*
 * game.h
 *
 *  Created on: Sep 12, 2025
 *      Author: jornik
 */

// game.h - Game Logic Header
// game.h - Main game controller
#ifndef GAME_H
#define GAME_H

#include "game_types.h"
#include <stdint.h>

// Initialize the game
void Game_Init(void);

// Main game update loop
void Game_Update(uint32_t current_time);

// Handle external inputs
void Game_HandleButton(uint8_t button_state, uint32_t current_time);
void Game_HandleKeyboard(uint8_t key);

// Game state management
void Game_Start(void);
void Game_Pause(void);
void Game_Resume(void);
void Game_Reset(void);
void Game_Over(void);

// Get game information
GameState* Game_GetState(void);
uint32_t Game_GetScore(void);
uint8_t Game_IsRunning(void);

#endif // GAME_H
