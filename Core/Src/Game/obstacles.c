#include "./Game/obstacles.h"
#include "./Game/shapes.h"
#include <stdlib.h>
#include <string.h>

// External UART for debugging
extern void UART_Printf(const char* format, ...);

// Obstacle pool
static uint8_t auto_spawn_enabled = 1;
static Obstacle obstacle_pool[MAX_OBSTACLES];
static float next_spawn_z;
static uint32_t obstacles_passed;

// Initialize obstacle system
void Obstacles_Init(void)
{
    Obstacles_Reset();

    // Spawn initial obstacles
    for(int i = 0; i < 3; i++)
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

            // Randomly choose shape
            int shape_type = rand() % 2;
            switch(shape_type)
            {
                case 0:
                    obs->shape_id = SHAPE_CUBE;
                    obs->width = 16;
                    obs->height = 16;
                    obs->depth = 16;
                    break;
                case 1:
                    obs->shape_id = SHAPE_CONE;
                    obs->width = 16;
                    obs->height = 25;
                    obs->depth = 16;
                    break;
                case 2:
                    obs->shape_id = SHAPE_PYRAMID;
                    obs->width = 20;
                    obs->height = 20;
                    obs->depth = 20;
                    break;
            }

            // Random X position within bounds
            int x_range = WORLD_MAX_X - WORLD_MIN_X;
            obs->pos.x = (float)(rand() % x_range) + WORLD_MIN_X;
            obs->pos.y = 0;
            obs->pos.z = z_position;

            UART_Printf("Spawned obstacle %d at [%d, %d]\r\n",
                       obs->shape_id, (int)obs->pos.x, (int)obs->pos.z);
            break;
        }
    }
}

// Update obstacles
void Obstacles_Update(float player_z, float delta_time)
{
    //uint8_t need_spawn = 0;
    float furthest_z = player_z;

    for(int i = 0; i < MAX_OBSTACLES; i++)
    {
        if(obstacle_pool[i].active)
        {
            // Remove obstacles that are far behind player
            if(obstacle_pool[i].pos.z < player_z - 30)
            {
                obstacle_pool[i].active = 0;
                obstacles_passed++;
                UART_Printf("Obstacle passed! Total: %lu\r\n", obstacles_passed);
            }

            // Track furthest obstacle
            if(obstacle_pool[i].pos.z > furthest_z)
            {
                furthest_z = obstacle_pool[i].pos.z;
            }
        }
    }

    // Check if we need to spawn new obstacles
    if(auto_spawn_enabled && furthest_z < player_z + OBSTACLE_SPAWN_DIST)
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
