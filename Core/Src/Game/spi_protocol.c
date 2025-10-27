#include "./Game/spi_protocol.h"
#include "./Utilities/transform.h"
#include <string.h>

// External UART for debugging
extern void UART_Printf(const char* format, ...);

// SPI handle pointer
static SPI_HandleTypeDef* hspi = NULL;

// SPI CS Pin
#define SPI_CS_PORT GPIOA
#define SPI_CS_PIN  GPIO_PIN_4

// Initialize SPI protocol
void SPI_Protocol_Init(SPI_HandleTypeDef* spi_handle)
{
    hspi = spi_handle;
}

// Low-level packet transmission
void SPI_TransmitPacket(uint8_t* data, uint16_t size)
{
    if(hspi == NULL) return;

    // Pull CS low
    HAL_GPIO_WritePin(SPI_CS_PORT, SPI_CS_PIN, GPIO_PIN_RESET);

    // Transmit data
    HAL_SPI_Transmit(hspi, data, size, 100);

    // Pull CS high
    HAL_GPIO_WritePin(SPI_CS_PORT, SPI_CS_PIN, GPIO_PIN_SET);
}

// Send player position
void SPI_SendPosition(Position* pos)
{
    PositionPacket packet;
    packet.cmd = CMD_POSITION;
    packet.x = (int16_t)pos->x;
    packet.y = (int16_t)pos->y;
    packet.z = (int16_t)pos->z;

    SPI_TransmitPacket((uint8_t*)&packet, sizeof(packet));

    #ifdef DEBUG_SPI
    UART_Printf("SPI: Position [%d,%d,%d]\r\n", packet.x, packet.y, packet.z);
    #endif
}

// Send shape definition to FPGA
void SPI_SendShape(Shape3D* shape)
{
    uint8_t packet[512];  // Large buffer for shape data
    uint16_t idx = 0;

    // Header
    packet[idx++] = CMD_SHAPE_DEF;
    packet[idx++] = shape->id;
    packet[idx++] = shape->vertex_count;
    packet[idx++] = shape->triangle_count;

    // Pack vertices (little-endian)
    for(int i = 0; i < shape->vertex_count; i++)
    {
        packet[idx++] = shape->vertices[i].x & 0xFF;
        packet[idx++] = (shape->vertices[i].x >> 8) & 0xFF;
        packet[idx++] = shape->vertices[i].y & 0xFF;
        packet[idx++] = (shape->vertices[i].y >> 8) & 0xFF;
        packet[idx++] = shape->vertices[i].z & 0xFF;
        packet[idx++] = (shape->vertices[i].z >> 8) & 0xFF;
    }

    // Pack triangles
    for(int i = 0; i < shape->triangle_count; i++)
    {
        packet[idx++] = shape->triangles[i].v1;
        packet[idx++] = shape->triangles[i].v2;
        packet[idx++] = shape->triangles[i].v3;
    }

    SPI_TransmitPacket(packet, idx);

    UART_Printf("SPI: Shape %d sent (%d bytes, %d verts, %d tris)\r\n",
               shape->id, idx, shape->vertex_count, shape->triangle_count);
}

// Updated: Now takes model_id as parameter
void SPI_SendShapeToFPGA(uint8_t model_id, Shape3D* shape)
{
    // Begin upload now takes model ID as parameter
    uint8_t begin_packet[2];
    begin_packet[0] = CMD_BEGIN_UPLOAD;
    begin_packet[1] = model_id;
    SPI_TransmitPacket(begin_packet, 2);

    // Upload triangles
    for(int i = 0; i < shape->triangle_count; i++) {
        uint8_t packet[39];
        packet[0] = CMD_UPLOAD_TRIANGLE;
        packet[1] = 0xFF;  // RGB byte 1
        packet[2] = 0xFF;  // RGB byte 2

        for(int v = 0; v < 3; v++) {
            uint8_t vertex_idx = (v == 0) ? shape->triangles[i].v1 :
                                (v == 1) ? shape->triangles[i].v2 :
                                          shape->triangles[i].v3;

            int32_t x = (int32_t)(shape->vertices[vertex_idx].x * 65536.0f);
            int32_t y = (int32_t)(shape->vertices[vertex_idx].y * 65536.0f);
            int32_t z = (int32_t)(shape->vertices[vertex_idx].z * 65536.0f);

            int offset = 3 + (v * 12);
            // Pack all three coordinates
            packet[offset+0] = (x >> 24) & 0xFF;
            packet[offset+1] = (x >> 16) & 0xFF;
            packet[offset+2] = (x >> 8) & 0xFF;
            packet[offset+3] = x & 0xFF;

            packet[offset+4] = (y >> 24) & 0xFF;
            packet[offset+5] = (y >> 16) & 0xFF;
            packet[offset+6] = (y >> 8) & 0xFF;
            packet[offset+7] = y & 0xFF;

            packet[offset+8] = (z >> 24) & 0xFF;
            packet[offset+9] = (z >> 16) & 0xFF;
            packet[offset+10] = (z >> 8) & 0xFF;
            packet[offset+11] = z & 0xFF;
        }

        SPI_TransmitPacket(packet, 39);
    }

    UART_Printf("SPI: Uploaded model ID %d with %d triangles\r\n",
                model_id, shape->triangle_count);
}

// Updated: Now includes is_last_model parameter
void SPI_AddModelInstance(uint8_t shape_id, Position* pos, float* rotation_matrix, uint8_t is_last_model)
{
    uint8_t packet[51];
    memset(packet, 0, 51);

    packet[0] = CMD_ADD_INSTANCE;
    packet[1] = is_last_model ? 0x01 : 0x00;  // Last model flag
    packet[2] = shape_id;

    // Position in fixed-point
    int32_t x_fixed = (int32_t)(pos->x * 65.5360f);
    int32_t y_fixed = (int32_t)(pos->y * 65.5360f);
    int32_t z_fixed = (int32_t)(pos->z * 65.5360f * 0);

    // Pack position
    packet[3] = (x_fixed >> 24) & 0xFF;
    packet[4] = (x_fixed >> 16) & 0xFF;
    packet[5] = (x_fixed >> 8) & 0xFF;
    packet[6] = x_fixed & 0xFF;

    packet[7] = (y_fixed >> 24) & 0xFF;
    packet[8] = (y_fixed >> 16) & 0xFF;
    packet[9] = (y_fixed >> 8) & 0xFF;
    packet[10] = y_fixed & 0xFF;

    packet[11] = (z_fixed >> 24) & 0xFF;
    packet[12] = (z_fixed >> 16) & 0xFF;
    packet[13] = (z_fixed >> 8) & 0xFF;
    packet[14] = z_fixed & 0xFF;

    // Pack rotation matrix (or identity if NULL)
    if(rotation_matrix != NULL) {
        // Use provided matrix
        for(int i = 0; i < 9; i++) {
            int32_t value = (int32_t)(rotation_matrix[i] * 65536.0f);
            int offset = 15 + (i * 4);
            packet[offset] = (value >> 24) & 0xFF;
            packet[offset+1] = (value >> 16) & 0xFF;
            packet[offset+2] = (value >> 8) & 0xFF;
            packet[offset+3] = value & 0xFF;
        }
    } else {
        // Identity matrix
        int32_t one = 65536;  // 1.0 in fixed point
        int32_t zero = 0;

        for(int i = 0; i < 9; i++) {
            int32_t value = (i == 0 || i == 4 || i == 8) ? one : zero;
            int offset = 15 + (i * 4);
            packet[offset] = (value >> 24) & 0xFF;
            packet[offset+1] = (value >> 16) & 0xFF;
            packet[offset+2] = (value >> 8) & 0xFF;
            packet[offset+3] = value & 0xFF;
        }
    }

    SPI_TransmitPacket(packet, 51);

    #ifdef DEBUG_SPI
    UART_Printf("SPI: Added instance of model %d at [%d,%d,%d]%s\r\n",
                    shape_id,
                    (int)pos->x,
                    (int)pos->y,
                    (int)pos->z,
                    is_last_model ? " (last model)" : "");
    #endif
}

// REMOVED: SPI_BeginRender() - no longer needed

// Send obstacle positions
void SPI_SendObstacles(Obstacle* obstacles, uint8_t count)
{
    uint8_t packet[256];
    uint16_t idx = 0;

    packet[idx++] = CMD_OBSTACLE_POS;
    packet[idx++] = 0;  // Count placeholder

    uint8_t sent_count = 0;

    for(int i = 0; i < count && sent_count < 15; i++)
    {
        if(obstacles[i].active)
        {
            packet[idx++] = obstacles[i].shape_id;

            // Position as int16
            int16_t x = (int16_t)obstacles[i].pos.x;
            int16_t y = (int16_t)obstacles[i].pos.y;
            int16_t z = (int16_t)obstacles[i].pos.z;

            // Pack position (little-endian)
            packet[idx++] = x & 0xFF;
            packet[idx++] = (x >> 8) & 0xFF;
            packet[idx++] = y & 0xFF;
            packet[idx++] = (y >> 8) & 0xFF;
            packet[idx++] = z & 0xFF;
            packet[idx++] = (z >> 8) & 0xFF;

            sent_count++;
        }
    }

    packet[1] = sent_count;  // Update count

    if(sent_count > 0)
    {
        SPI_TransmitPacket(packet, idx);

        #ifdef DEBUG_SPI
        UART_Printf("SPI: Sent %d obstacles\r\n", sent_count);
        #endif
    }
}

// Send collision event
void SPI_SendCollisionEvent(void)
{
    EventPacket packet;
    packet.cmd = CMD_COLLISION;
    packet.event_type = 0x01;  // Collision occurred

    SPI_TransmitPacket((uint8_t*)&packet, sizeof(packet));

    UART_Printf("SPI: Collision event sent\r\n");
}

// Send game state update
void SPI_SendGameState(GameStateEnum state, uint32_t score)
{
    uint8_t packet[8];
    packet[0] = CMD_GAME_STATE;
    packet[1] = (uint8_t)state;

    // Pack score (little-endian)
    packet[2] = score & 0xFF;
    packet[3] = (score >> 8) & 0xFF;
    packet[4] = (score >> 16) & 0xFF;
    packet[5] = (score >> 24) & 0xFF;

    SPI_TransmitPacket(packet, 6);

    UART_Printf("SPI: Game state=%d, score=%lu\r\n", state, score);
}

// Clear scene command
void SPI_ClearScene(void)
{
    uint8_t packet[2] = {CMD_CLEAR_SCENE, 0x00};
    SPI_TransmitPacket(packet, 2);

    UART_Printf("SPI: Clear scene sent\r\n");
}
