#include "./Game/collision.h"
#include "./Game/shapes.h"
#include <math.h>

// Player collision box dimensions
#define PLAYER_WIDTH  2.0f
#define PLAYER_HEIGHT 3.0f
#define PLAYER_DEPTH  3.0f

// Check player collision with obstacles
CollisionResult Collision_CheckPlayer(Position* player_pos, Obstacle* obstacles, uint8_t obstacle_count)
{
    CollisionResult result = {COLLISION_NONE, 0, 0.0f};

    // // Check boundary collision
    // if(player_pos->x < WORLD_MIN_X || player_pos->x > WORLD_MAX_X)
    // {
    //     result.type = COLLISION_BOUNDARY;
    //     return result;
    // }

    // Check obstacle collisions
    for(uint8_t i = 0; i < obstacle_count; i++)
    {
        if(obstacles[i].active)
        {
            if(Collision_BoxIntersect(player_pos, Shapes_GetPlayer()->width, Shapes_GetPlayer()->height, Shapes_GetPlayer()->depth,
                                     &obstacles[i].pos, obstacles[i].width,
                                     obstacles[i].height, obstacles[i].depth-1))
            {
                result.type = COLLISION_OBSTACLE;
                result.obstacle_index = i;

                // Calculate penetration depth for physics response
                float dx = fabsf(player_pos->x - obstacles[i].pos.x);
                float dy = fabsf(player_pos->y - obstacles[i].pos.y);
                float dz = fabsf(player_pos->z - obstacles[i].pos.z);

                float px = (Shapes_GetPlayer()->width/2 + obstacles[i].width/2) - dx;
                float py = (Shapes_GetPlayer()->height/2 + obstacles[i].height/2) - dy;
                float pz = (Shapes_GetPlayer()->depth/2 + obstacles[i].depth/2) - dz;

                // Find minimum penetration
                result.penetration_depth = px;
                if(py < result.penetration_depth) result.penetration_depth = py;
                if(pz < result.penetration_depth) result.penetration_depth = pz;

                // Debug print: player and obstacle positions and dimensions
                UART_Printf("PLAYER: pos=(%d, %d, %d) w=%d h=%d d=%d\r\n",
                    (int)player_pos->x, (int)player_pos->y, (int)player_pos->z,
                    (int)Shapes_GetPlayer()->width, (int)Shapes_GetPlayer()->height, (int)Shapes_GetPlayer()->depth);
                UART_Printf("OBSTACLE: pos=(%d, %d, %d) w=%d h=%d d=%d\r\n",
                    (int)obstacles[i].pos.x, (int)obstacles[i].pos.y, (int)obstacles[i].pos.z,
                    (int)obstacles[i].width, (int)obstacles[i].height, (int)obstacles[i].depth);


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
    // More forgiving on X-axis, normal on Z-axis
    float x_scale = 0.7f;  // 70% of actual size for X
    float y_scale = 0.8f;  // 80% for Y
    float z_scale = 0.9f;  // 90% for Z (front/back)

    float hw1 = (w1 / 2.0f) * x_scale;
    float hh1 = (h1 / 2.0f) * y_scale;
    float hd1 = (d1 / 2.0f) * z_scale;
    float hw2 = (w2 / 2.0f) * x_scale;
    float hh2 = (h2 / 2.0f) * y_scale;
    float hd2 = (d2 / 2.0f) * z_scale;

    // Check overlap on all axes
    if(fabsf(pos1->x - pos2->x) >= (hw1 + hw2)) return 0;
    if(fabsf(pos1->y - pos2->y) >= (hh1 + hh2)) return 0;
    if(fabsf(pos1->z - pos2->z) >= (hd1 + hd2)) return 0;

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
        player_pos->z -= result->penetration_depth;
    }
}
