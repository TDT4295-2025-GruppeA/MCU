#ifndef OBSTACLES_H
#define OBSTACLES_H

#include "game_types.h"

// Initialize obstacle system
void Obstacles_Init(void);

// Obstacle management
void Obstacles_Update(Position* player_pos, float delta_time);
void Obstacles_Spawn(float z_position);
void Obstacles_Clear(void);
void Obstacles_Reset(void);
void Obstacles_SetAutoSpawn(uint8_t enabled);
void Obstacles_MoveTowardPlayer(float speed);

// Getters
Obstacle* Obstacles_GetArray(void);
uint8_t Obstacles_GetActiveCount(void);
uint8_t Obstacles_GetVisibleCount(float player_z, float view_distance);

// Scoring
uint32_t Obstacles_CheckPassed(float player_z);

#endif // OBSTACLES_H
