#include "../../../Inc/Game/Rendering/rendering.h"
#include "../../../Inc/Game/spi_protocol.h"
#include "../../../Inc/Game/shapes.h"
#include "../../../Inc/Game/obstacles.h"
#include "../../../Inc/Utilities/transform.h"
#include "main.h"

extern void UART_Printf(const char* format, ...);

static SPI_HandleTypeDef* spi_handle = NULL;

void Renderer_Init(SPI_HandleTypeDef* hspi)
{
    spi_handle = hspi;
    SPI_Protocol_Init(hspi);
    UART_Printf("Renderer initialized\r\n");
}

void Renderer_UploadShapes(void)
{
    UART_Printf("Uploading shapes to FPGA...\r\n");

    // Upload all game shapes with their IDs
    SPI_SendShapeToFPGA(SHAPE_ID_PLAYER, Shapes_GetPlayer());
    SPI_SendShapeToFPGA(SHAPE_CUBE, Shapes_GetCube());
    SPI_SendShapeToFPGA(SHAPE_CONE, Shapes_GetCone());

    UART_Printf("Shapes uploaded successfully\r\n");
}

void Renderer_DrawFrame(GameState* state)
{
    if(!state) return;

    // 1. Render player with banking effect
    Matrix3x3 player_rotation;
    float tilt_angle = state->player_pos.x * 0.01f;
    Matrix_RotateZ(&player_rotation, tilt_angle);

    SPI_AddModelInstance(SHAPE_ID_PLAYER, &state->player_pos,
                        player_rotation.m, 0);  // Not last model

    // 2. Get obstacles and count visible ones
    Obstacle* obstacles = Obstacles_GetArray();
    int visible_count = 0;

    for(int i = 0; i < MAX_OBSTACLES; i++) {
        if(obstacles[i].active) {
            float rel_z = obstacles[i].pos.z - state->player_pos.z;
            if(rel_z > -20 && rel_z < 150) {
                visible_count++;
            }
        }
    }

    // 3. Render visible obstacles
    int rendered = 0;
    uint8_t is_last_model = 0;

    for(int i = 0; i < MAX_OBSTACLES && rendered < 15; i++) {
        if(!obstacles[i].active) continue;

        // Calculate relative position for camera
        Position relative_pos = obstacles[i].pos;
        relative_pos.z -= state->player_pos.z;

        // Check if visible
        if(relative_pos.z > -20 && relative_pos.z < 150) {
            rendered++;
            is_last_model = (rendered >= visible_count || rendered >= 15) ? 1 : 0;

            // Apply rotation based on shape type
            Matrix3x3 rotation;
            if(obstacles[i].shape_id == SHAPE_CUBE) {
                float angle = (HAL_GetTick() * 0.001f) + (i * 0.5f);
                Matrix_RotateY(&rotation, angle);
            } else if(obstacles[i].shape_id == SHAPE_CONE) {
                Matrix_RotateX(&rotation, 0.2f);
            } else {
                Matrix_Identity(&rotation);
            }

            SPI_AddModelInstance(obstacles[i].shape_id, &relative_pos,
                               rotation.m, is_last_model);

            if(is_last_model) break;
        }
    }

    // 4. If no obstacles were rendered, send player again as last model
    if(rendered == 0) {
        SPI_AddModelInstance(SHAPE_ID_PLAYER, &state->player_pos,
                           player_rotation.m, 1);  // Last model
    }
}

void Renderer_ClearScene(void)
{
}
