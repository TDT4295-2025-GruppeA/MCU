/*
 * spi_protocol.h
 *
 *  Created on: Sep 15, 2025
 *      Author: jornik
 */

// spi_protocol.h - SPI communication protocol with FPGA
#ifndef SPI_PROTOCOL_H
#define SPI_PROTOCOL_H

#include "game_types.h"
#include "stm32u5xx_hal.h"

// Enable SIM_UART to mirror SPI packets to USART1 (ST-LINK VCP) for the PC simulator.
// Comment out or remove this line to disable simulator forwarding in production builds.
#define SIM_UART


// Initialize SPI protocol handler
void SPI_Protocol_Init(SPI_HandleTypeDef* spi_handle);
void SPI_TransmitPacket(uint8_t* data, uint16_t size);

// Protocol commands (see documentation/spi_protocol.md)
void SPI_SendReset(void);
uint8_t SPI_BeginUpload(void);
void SPI_UploadTriangle(uint16_t color, float v0[3], float v1[3], float v2[3]);
void SPI_AddModelInstance(uint8_t model_id, float pos[3], float rot[9]);
void SPI_MarkFrameStart(void);
void SPI_MarkFrameEnd(void);

#endif // SPI_PROTOCOL_H
