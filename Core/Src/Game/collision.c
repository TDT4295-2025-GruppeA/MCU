/*
 * collision.c
 *
 *  Created on: Sep 15, 2025
 *      Author: jornik
 */

// collision.c - Collision detection implementation
#include "./Game/collision.h"
#include <math.h>

// Player collision box dimensions
#define PLAYER_WIDTH  10.0f
#define PLAYER_HEIGHT 5.0f
#define PLAYER_DEPTH  10.0f

// Check player collision with obstacles
CollisionResult Collision_CheckPlayer(Position* player_pos, Obstacle* obstacles, uint8_t obstacle_count)
{
    CollisionResult result = {COLLISION_NONE, 0, 0.0f};

    // Check boundary collision
    if(player_pos->x < WORLD_MIN_X || player_pos->x > WORLD_MAX_X)
    {
        result.type = COLLISION_BOUNDARY;
        return result;
    }

    // Check obstacle collisions
    for(uint8_t i = 0; i < obstacle_count; i++)
    {
        if(obstacles[i].active)
        {
            if(Collision_BoxIntersect(player_pos, PLAYER_WIDTH, PLAYER_HEIGHT, PLAYER_DEPTH,
                                     &obstacles[i].pos, obstacles[i].width,
                                     obstacles[i].height, obstacles[i].depth))
            {
                result.type = COLLISION_OBSTACLE;
                result.obstacle_index = i;

                // Calculate penetration depth for physics response
                float dx = fabsf(player_pos->x - obstacles[i].pos.x);
                float dy = fabsf(player_pos->y - obstacles[i].pos.y);
                float dz = fabsf(player_pos->z - obstacles[i].pos.z);

                float px = (PLAYER_WIDTH/2 + obstacles[i].width/2) - dx;
                float py = (PLAYER_HEIGHT/2 + obstacles[i].height/2) - dy;
                float pz = (PLAYER_DEPTH/2 + obstacles[i].depth/2) - dz;

                // Find minimum penetration
                result.penetration_depth = px;
                if(py < result.penetration_depth) result.penetration_depth = py;
                if(pz < result.penetration_depth) result.penetration_depth = pz;

                return result;
            }
        }
    }

    return result;
}

// Check if two boxes intersect
uint8_t Collision_BoxIntersect(Position* pos1, float w1, float h1, float d1,
                               Position* pos2, float w2, float h2, float d2)
{
    // Calculate half-sizes
    float hw1 = w1 / 2.0f;
    float hh1 = h1 / 2.0f;
    float hd1 = d1 / 2.0f;
    float hw2 = w2 / 2.0f;
    float hh2 = h2 / 2.0f;
    float hd2 = d2 / 2.0f;

    // Check overlap on all axes
    if(fabsf(pos1->x - pos2->x) > (hw1 + hw2)) return 0;
    if(fabsf(pos1->y - pos2->y) > (hh1 + hh2)) return 0;
    if(fabsf(pos1->z - pos2->z) > (hd1 + hd2)) return 0;

    return 1;  // Collision detected
}

// Calculate distance from point to box
float Collision_PointToBoxDistance(Position* point, Position* box_center,
                                   float box_width, float box_height, float box_depth)
{
    float hw = box_width / 2.0f;
    float hh = box_height / 2.0f;
    float hd = box_depth / 2.0f;

    // Find closest point on box to the point
    float closest_x = point->x;
    float closest_y = point->y;
    float closest_z = point->z;

    // Clamp to box bounds
    if(closest_x < box_center->x - hw) closest_x = box_center->x - hw;
    if(closest_x > box_center->x + hw) closest_x = box_center->x + hw;
    if(closest_y < box_center->y - hh) closest_y = box_center->y - hh;
    if(closest_y > box_center->y + hh) closest_y = box_center->y + hh;
    if(closest_z < box_center->z - hd) closest_z = box_center->z - hd;
    if(closest_z > box_center->z + hd) closest_z = box_center->z + hd;

    // Calculate distance
    float dx = point->x - closest_x;
    float dy = point->y - closest_y;
    float dz = point->z - closest_z;

    return sqrtf(dx*dx + dy*dy + dz*dz);
}

// Resolve collision (push player away)
void Collision_ResolvePlayer(Position* player_pos, CollisionResult* result)
{
    if(result->type == COLLISION_BOUNDARY)
    {
        // Clamp to world bounds
        if(player_pos->x < WORLD_MIN_X) player_pos->x = WORLD_MIN_X;
        if(player_pos->x > WORLD_MAX_X) player_pos->x = WORLD_MAX_X;
    }
    else if(result->type == COLLISION_OBSTACLE && result->penetration_depth > 0)
    {
        // Simple resolution: stop forward movement or push back
        player_pos->z -= result->penetration_depth;
    }
}
