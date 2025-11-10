
#ifndef GAME_H
#define GAME_H

#include "game_types.h"
#include <stdint.h>

void Game_Init(void);
void Game_Update(uint32_t current_time);

void Game_Reset(void);
void Game_Pause(void);
void Game_Resume(void);
void Game_Start(void);
void Game_Over(void);

// Game state queries
GameState* Game_GetState(void);
uint32_t Game_GetScore(void);
uint8_t Game_IsRunning(void);

#endif // GAME_H
