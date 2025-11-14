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
    uint8_t reset_data[] = {0x55, 0x55};
    SPI_TransmitPacket(reset_data, 2);
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
    uint8_t padding_data[] = {0xFF, 0xFF};
    SPI_TransmitPacket(padding_data, 2);

    // 1. Render player at origin with banking
    Matrix3x3 player_rotation;
    float tilt_angle = state->player_pos.x * 0.01f;
    float angle = (HAL_GetTick() * 0.001f) + (1 * 0.5f);

    Matrix_RotateZ(&player_rotation, tilt_angle);
    state->player_pos.z = 4;
    Position player_render_pos = {state->player_pos.x, 1, state->player_pos.z};
    SPI_AddModelInstance(SHAPE_ID_PLAYER, &player_render_pos,
                        player_rotation.m, 0);

    // 2. Count visible obstacles
    Obstacle* obstacles = Obstacles_GetArray();
    int visible_count = 0;

    for(int i = 0; i < MAX_OBSTACLES; i++) {
        if(obstacles[i].active &&
           obstacles[i].pos.z > -20 && obstacles[i].pos.z < 150) {
            visible_count++;
        }
    }

    // 3. Render obstacles
    int rendered = 0;
    uint8_t is_last_model = 0;



    for(int i = 0; i < MAX_OBSTACLES && rendered < 25; i++) {
        if(!obstacles[i].active) continue;

        // Check if visible
        if(obstacles[i].pos.z > -20 && obstacles[i].pos.z < 150) {
            rendered++;
            is_last_model = (rendered >= visible_count || rendered >= 25) ? 1 : 0;

            // Apply rotation
            Matrix3x3 rotation;
            if(obstacles[i].shape_id == SHAPE_CUBE) {
                float angle = (HAL_GetTick() * 0.001f) + (i * 0.5f);
                Matrix_RotateY(&rotation, angle);
            } else {
                Matrix_Identity(&rotation);
            }

            // Send actual position
            SPI_AddModelInstance(obstacles[i].shape_id, &obstacles[i].pos,
                               rotation.m, is_last_model);

            if(is_last_model) break;
        }
    }

    // 4. Fallback if no obstacles
    if(rendered == 0) {
        SPI_AddModelInstance(SHAPE_ID_PLAYER, &player_render_pos,
                           player_rotation.m, 1);
    }
}

void Renderer_ClearScene(void)
{
    SPI_ClearScene();
}

void Renderer_SendCollisionEffect(void)
{
    SPI_SendCollisionEvent();
}
