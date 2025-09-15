/*
 * collision.h
 *
 *  Created on: Sep 15, 2025
 *      Author: jornik
 */

// collision.h - Collision detection system
#ifndef COLLISION_H
#define COLLISION_H

#include "game_types.h"

// Collision detection types
typedef enum {
    COLLISION_NONE = 0,
    COLLISION_OBSTACLE,
    COLLISION_BOUNDARY,
    COLLISION_POWERUP
} CollisionType;

// Collision result structure
typedef struct {
    CollisionType type;
    uint8_t obstacle_index;
    float penetration_depth;
} CollisionResult;

// Collision detection functions
CollisionResult Collision_CheckPlayer(Position* player_pos, Obstacle* obstacles, uint8_t obstacle_count);
uint8_t Collision_BoxIntersect(Position* pos1, float w1, float h1, float d1,
                               Position* pos2, float w2, float h2, float d2);
float Collision_PointToBoxDistance(Position* point, Position* box_center,
                                   float box_width, float box_height, float box_depth);

// Collision response
void Collision_ResolvePlayer(Position* player_pos, CollisionResult* result);

#endif // COLLISION_H
