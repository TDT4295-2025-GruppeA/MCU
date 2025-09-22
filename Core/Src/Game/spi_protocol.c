/*
 * spi_protocol.c
 *
 *  Created on: Sep 15, 2025
 *      Author: jornik
 */

// spi_protocol.c - SPI communication implementation
#include "./Game/spi_protocol.h"
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
