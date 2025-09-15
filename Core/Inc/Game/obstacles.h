/*
 * obstacles.h
 *
 *  Created on: Sep 15, 2025
 *      Author: jornik
 */

// obstacles.h - Obstacle spawning and management
#ifndef OBSTACLES_H
#define OBSTACLES_H

#include "game_types.h"

// Initialize obstacle system
void Obstacles_Init(void);

// Obstacle management
void Obstacles_Update(float player_z, float delta_time);
void Obstacles_Spawn(float z_position);
void Obstacles_Clear(void);
void Obstacles_Reset(void);

// Getters
Obstacle* Obstacles_GetArray(void);
uint8_t Obstacles_GetActiveCount(void);
uint8_t Obstacles_GetVisibleCount(float player_z, float view_distance);

// Scoring
uint32_t Obstacles_CheckPassed(float player_z);

#endif // OBSTACLES_H
