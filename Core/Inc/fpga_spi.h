/*
 * fpga_spi.h
 *
 *  Created on: Sep 8, 2025
 *      Author: jornik
 */

#ifndef FPGA_SPI_H
#define FPGA_SPI_H

#include "main.h"
#include <string.h>

// SPI Configuration
#define FPGA_SPI_INSTANCE     hspi1
#define FPGA_CS_PORT          GPIOA
#define FPGA_CS_PIN           GPIO_PIN_4
#define SPI_TIMEOUT_MS        100

// Protocol definitions for FPGA commands
typedef enum {
    // Graphics commands
    CMD_NOP           = 0x00,
    CMD_CLEAR_SCREEN  = 0x01,
    CMD_DRAW_PIXEL    = 0x02,
    CMD_DRAW_LINE     = 0x03,
    CMD_DRAW_RECT     = 0x04,
    CMD_DRAW_TRIANGLE = 0x05,
    CMD_DRAW_CIRCLE   = 0x06,
    CMD_DRAW_CUBE     = 0x10,
    CMD_DRAW_SPHERE   = 0x11,
    CMD_SET_COLOR     = 0x20,
    CMD_SET_CAMERA    = 0x21,
    CMD_SWAP_BUFFER   = 0x30,

    // System commands
    CMD_READ_STATUS   = 0x80,
    CMD_READ_VERSION  = 0x81,
    CMD_RESET         = 0xFF
} FPGA_Command;

// Color structure (RGB888)
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color;

// 2D Point
typedef struct {
    int16_t x;
    int16_t y;
} Point2D;

// 3D Point
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} Point3D;

// Camera/viewport settings
typedef struct {
    Point3D position;
    Point3D rotation;  // Euler angles
    uint16_t fov;      // Field of view
} Camera;

// Status response from FPGA
typedef struct {
    uint8_t ready;
    uint8_t buffer_full;
    uint8_t error_code;
    uint16_t fps;
} FPGA_Status;

// Initialize SPI communication with FPGA
HAL_StatusTypeDef FPGA_SPI_Init(SPI_HandleTypeDef* hspi);

// Basic drawing commands
HAL_StatusTypeDef FPGA_ClearScreen(Color color);
HAL_StatusTypeDef FPGA_DrawPixel(int16_t x, int16_t y, Color color);
HAL_StatusTypeDef FPGA_DrawLine(Point2D start, Point2D end, Color color);
HAL_StatusTypeDef FPGA_DrawRect(Point2D topLeft, uint16_t width, uint16_t height, Color color, uint8_t filled);
HAL_StatusTypeDef FPGA_DrawTriangle(Point2D p1, Point2D p2, Point2D p3, Color color, uint8_t filled);

// 3D drawing commands
HAL_StatusTypeDef FPGA_DrawCube(Point3D center, uint16_t size, Color color);
HAL_StatusTypeDef FPGA_DrawSphere(Point3D center, uint16_t radius, Color color);
HAL_StatusTypeDef FPGA_SetCamera(Camera* cam);

// Buffer management
HAL_StatusTypeDef FPGA_SwapBuffers(void);

// System commands
HAL_StatusTypeDef FPGA_GetStatus(FPGA_Status* status);
HAL_StatusTypeDef FPGA_Reset(void);

// Low-level SPI functions
HAL_StatusTypeDef FPGA_SendCommand(uint8_t cmd, uint8_t* data, uint16_t len);
HAL_StatusTypeDef FPGA_ReadData(uint8_t cmd, uint8_t* buffer, uint16_t len);

#endif
