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
    SPI_SendShapeToFPGA(SHAPE_GROUND, Shapes_GetGround());
    SPI_SendShapeToFPGA(SHAPE_ID_PLAYER, Shapes_GetPlayer());
    SPI_SendShapeToFPGA(SHAPE_CUBE, Shapes_GetCube());
    SPI_SendShapeToFPGA(SHAPE_CONE, Shapes_GetCone());

    UART_Printf("Shapes uploaded successfully\r\n");
}

void Renderer_DrawFrame(GameState* state)
{
    if(!state) return;
    
    // This helps for some reasone.
    // Maybe it clears out garbage data on FPGA side?
    uint8_t reset_data[] = {0x00, 0x00, 0x00, 0x00};
    SPI_TransmitPacket(reset_data, 4);

    Position camera_pos = {0, 2.5, 6};
    Matrix3x3 cam_tilt, cam_roll, cam_rot;
    Matrix_RotateX(&cam_tilt, 0.15f);
    float camera_roll_angle = -state->player_strafe_speed / PLAYER_STRAFE_MAX_SPEED / 4;
    Matrix_RotateZ(&cam_roll, camera_roll_angle);
    Matrix_Multiply(&cam_rot, &cam_roll, &cam_tilt);
    SPI_SetCameraPosition(&camera_pos, cam_rot.m);
    
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

    for(int i = 0; i < MAX_OBSTACLES && rendered < 15; i++) {
        if(!obstacles[i].active) continue;

        // Check if visible
        if(obstacles[i].pos.z > -20 && obstacles[i].pos.z < 150) {
            rendered++;
            is_last_model = (rendered >= visible_count || rendered >= 15) ? 1 : 0;

            // Apply rotation
            Matrix3x3 rotation;
            if(obstacles[i].shape_id == SHAPE_CUBE) {
                float angle = (HAL_GetTick() * 0.001f) + (i * 0.5f);
                Matrix_RotateY(&rotation, angle);
            } else {
                Matrix_Identity(&rotation);
            }

            // Adjust obstacle X to keep player visually centered
            Position render_pos = obstacles[i].pos;
            render_pos.x -= state->player_pos.x;

            SPI_AddModelInstance(obstacles[i].shape_id, &render_pos,
                               rotation.m, 0);

            if(is_last_model) break;
        }
    }

    // Render ground plane at origin
    Shape3D* ground = Shapes_GetGround();
    Position ground_pos = {0, 0, 60};
    SPI_AddModelInstance(ground->id, &ground_pos, NULL, 0);

    // Render player at origin with banking
    Matrix3x3 player_rotation;
    float player_roll_angle = -camera_roll_angle*2;
    Matrix_RotateZ(&player_rotation, player_roll_angle);

    Position player_render_pos = {0, 0, 0};
    SPI_AddModelInstance(SHAPE_ID_PLAYER, &player_render_pos,
                        player_rotation.m, 1);
}

void Renderer_ClearScene(void)
{
}
