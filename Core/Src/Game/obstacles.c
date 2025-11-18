#include "./Game/obstacles.h"
#include "./Game/shapes.h"
#include "./Game/game.h"
#include <stdlib.h>
#include <string.h>

// External UART for debugging
extern void UART_Printf(const char* format, ...);

// Obstacle pool
static uint8_t auto_spawn_enabled = 1;
static Obstacle obstacle_pool[MAX_OBSTACLES];
static float next_spawn_z;
static uint32_t obstacles_passed;

// Pre-calculated constants for efficiency
static const int OBSTACLE_X_RANGE = WORLD_MAX_X - WORLD_MIN_X;

// Initialize obstacle system
void Obstacles_Init(void)
{
    Obstacles_Reset();

    // Spawn initial obstacles ahead of player (positive Z)
    for(int i = 0; i < MAX_OBSTACLES; i++)
    {
    Obstacles_Spawn(OBSTACLE_SPAWN_DIST + (i * OBSTACLE_SPACING));
    }
}

void Obstacles_SetAutoSpawn(uint8_t enabled) {
    auto_spawn_enabled = enabled;
}

// Reset all obstacles
void Obstacles_Reset(void)
{
    memset(obstacle_pool, 0, sizeof(obstacle_pool));
    next_spawn_z = OBSTACLE_SPAWN_DIST;
    obstacles_passed = 0;
}

// Clear all active obstacles
void Obstacles_Clear(void)
{
    for(int i = 0; i < MAX_OBSTACLES; i++)
    {
        obstacle_pool[i].active = 0;
    }
}

// Spawn a new obstacle
void Obstacles_Spawn(float z_position)
{
    // Find inactive slot
    for(int i = 0; i < MAX_OBSTACLES; i++)
    {
        if(!obstacle_pool[i].active)
        {
            Obstacle* obs = &obstacle_pool[i];
            obs->active = 1;

            // Set obstacle properties using shape bounds
            obs->shape_id = SHAPE_CUBE;
            Shape3D* cube_shape = Shapes_GetCube();
            obs->width = cube_shape->width;
            obs->height = cube_shape->height;
            obs->depth = cube_shape->depth;

            // Random X position relative to player
            const GameState* state = Game_GetState();
            float offset = ((float)(rand() % ((int)(2 * OBSTACLE_SPAWN_OFFSET)))) - OBSTACLE_SPAWN_OFFSET;
            obs->pos.x = state->player_pos.x + offset;
            obs->pos.y = 0;
            obs->pos.z = z_position;

            UART_Printf("Spawned obstacle %d at [%.1f, %.1f]\r\n",
                       obs->shape_id, obs->pos.x, obs->pos.z);
            break;
        }
    }
}

void Obstacles_MoveTowardPlayer(float speed)
{
    for(int i = 0; i < MAX_OBSTACLES; i++)
    {
        if(obstacle_pool[i].active)
        {
            obstacle_pool[i].pos.z -= speed;  // Move toward player (decrease Z)
        }
    }
}

// Update obstacles
void Obstacles_Update(Position* player_pos, float delta_time)
{
    float furthest_z = 0;  // Start at player position

    for(int i = 0; i < MAX_OBSTACLES; i++)
    {
        if(obstacle_pool[i].active)
        {
            // Remove obstacles that have passed behind the player
            if(obstacle_pool[i].pos.z < -1)  // Behind player
            {
                obstacle_pool[i].active = 0;
                obstacles_passed++;
                UART_Printf("Obstacle passed! Total: %lu\r\n", obstacles_passed);
            }

            // Track furthest obstacle (highest Z value)
            if(obstacle_pool[i].pos.z > furthest_z)
            {
                furthest_z = obstacle_pool[i].pos.z;
            }
        }
    }

    // Spawn new obstacles ahead when needed
    if(auto_spawn_enabled && furthest_z < OBSTACLE_SPAWN_DIST)
    {
        next_spawn_z = furthest_z + OBSTACLE_SPACING + (rand() % 20);
        Obstacles_Spawn(next_spawn_z);
    }
}

// Get obstacle array
Obstacle* Obstacles_GetArray(void)
{
    return obstacle_pool;
}

// Get active obstacle count
uint8_t Obstacles_GetActiveCount(void)
{
    uint8_t count = 0;
    for(int i = 0; i < MAX_OBSTACLES; i++)
    {
        if(obstacle_pool[i].active) count++;
    }
    return count;
}

// Get visible obstacle count
uint8_t Obstacles_GetVisibleCount(float player_z, float view_distance)
{
    uint8_t count = 0;
    for(int i = 0; i < MAX_OBSTACLES; i++)
    {
        if(obstacle_pool[i].active)
        {
            float dist = obstacle_pool[i].pos.z - player_z;
            if(dist > -10 && dist < view_distance)
            {
                count++;
            }
        }
    }
    return count;
}

// Check how many obstacles were passed
uint32_t Obstacles_CheckPassed(float player_z)
{
    return obstacles_passed;
}
