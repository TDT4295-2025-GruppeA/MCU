#ifndef SPI_PROTOCOL_H
#define SPI_PROTOCOL_H

#include "game_types.h"
#include "stm32u5xx_hal.h"

// Enable SIM_UART to mirror SPI packets to USART1 (ST-LINK VCP) for the PC simulator.
// Comment out or remove this line to disable simulator forwarding in production builds.
#define SIM_UART
// Command definitions
#define CMD_RESET           0x00
#define CMD_BEGIN_UPLOAD    0xA0
#define CMD_UPLOAD_TRIANGLE 0xA1
#define CMD_ADD_INSTANCE    0xB0

// Initialize SPI protocol handler
void SPI_Protocol_Init(SPI_HandleTypeDef* spi_handle);
void SPI_TransmitPacket(uint8_t* data, uint16_t size);

// Protocol commands (see documentation/spi_protocol.md)
void SPI_SendReset(void);
void SPI_SendShapeToFPGA(uint8_t model_id, Shape3D* shape);  //Updated to: includes model_id parameter
void SPI_AddModelInstance(uint8_t shape_id, Position* pos, float* rotation_matrix, uint8_t is_last_model);  // Added is_last_model parameter
#endif // SPI_PROTOCOL_H
