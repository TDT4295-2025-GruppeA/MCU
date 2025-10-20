// spi_protocol.h - SPI communication protocol with FPGA
#ifndef SPI_PROTOCOL_H
#define SPI_PROTOCOL_H

#include "game_types.h"
#include "stm32u5xx_hal.h"

#define CMD_RESET           0x00
#define CMD_BEGIN_UPLOAD    0xA0
#define CMD_UPLOAD_TRIANGLE 0xA1
#define CMD_ADD_INSTANCE    0xB0
#define CMD_BEGIN_RENDER    0xF0

// SPI packet structures
typedef struct {
    uint8_t cmd;
    int16_t x;
    int16_t y;
    int16_t z;
} __attribute__((packed)) PositionPacket;

typedef struct {
    uint8_t cmd;
    uint8_t event_type;
} __attribute__((packed)) EventPacket;

// Initialize SPI protocol handler
void SPI_Protocol_Init(SPI_HandleTypeDef* spi_handle);

// Send functions
void SPI_SendPosition(Position* pos);
void SPI_SendShape(Shape3D* shape);
void SPI_SendObstacles(Obstacle* obstacles, uint8_t count);
void SPI_SendCollisionEvent(void);
void SPI_SendGameState(GameStateEnum state, uint32_t score);
void SPI_ClearScene(void);
void SPI_SendShapeToFPGA(Shape3D* shape);
void SPI_AddModelInstance(uint8_t shape_id, Position* pos, float* rotation_matrix);
void SPI_BeginRender(void);

// Low-level SPI functions
void SPI_TransmitPacket(uint8_t* data, uint16_t size);

#endif // SPI_PROTOCOL_H
