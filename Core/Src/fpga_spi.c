#include "fpga_spi.h"

static SPI_HandleTypeDef* fpga_spi = NULL;

// Helper function to control CS pin
static void CS_Low(void) {
    HAL_GPIO_WritePin(FPGA_CS_PORT, FPGA_CS_PIN, GPIO_PIN_RESET);
}

static void CS_High(void) {
    HAL_GPIO_WritePin(FPGA_CS_PORT, FPGA_CS_PIN, GPIO_PIN_SET);
}

HAL_StatusTypeDef FPGA_SPI_Init(SPI_HandleTypeDef* hspi) {
    fpga_spi = hspi;

    // Initialize CS pin as output
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();  // Enable clock for CS port

    GPIO_InitStruct.Pin = FPGA_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(FPGA_CS_PORT, &GPIO_InitStruct);

    // Start with CS high (inactive)
    CS_High();

    return HAL_OK;
}

HAL_StatusTypeDef FPGA_SendCommand(uint8_t cmd, uint8_t* data, uint16_t len) {
    HAL_StatusTypeDef status;

    if(fpga_spi == NULL) return HAL_ERROR;

    CS_Low();

    // Send command byte
    status = HAL_SPI_Transmit(fpga_spi, &cmd, 1, SPI_TIMEOUT_MS);
    if(status != HAL_OK) {
        CS_High();
        return status;
    }

    // Send data if any
    if(data != NULL && len > 0) {
        status = HAL_SPI_Transmit(fpga_spi, data, len, SPI_TIMEOUT_MS);
    }

    CS_High();

    // Small delay to ensure FPGA processes command
    HAL_Delay(1);

    return status;
}

HAL_StatusTypeDef FPGA_ReadData(uint8_t cmd, uint8_t* buffer, uint16_t len) {
    HAL_StatusTypeDef status;

    if(fpga_spi == NULL || buffer == NULL) return HAL_ERROR;

    CS_Low();

    // Send read command
    status = HAL_SPI_Transmit(fpga_spi, &cmd, 1, SPI_TIMEOUT_MS);
    if(status != HAL_OK) {
        CS_High();
        return status;
    }

    // Read response
    status = HAL_SPI_Receive(fpga_spi, buffer, len, SPI_TIMEOUT_MS);

    CS_High();

    return status;
}

// Graphics command implementations
HAL_StatusTypeDef FPGA_ClearScreen(Color color) {
    uint8_t data[3] = {color.r, color.g, color.b};
    return FPGA_SendCommand(CMD_CLEAR_SCREEN, data, 3);
}

HAL_StatusTypeDef FPGA_DrawPixel(int16_t x, int16_t y, Color color) {
    uint8_t data[7];

    // Pack coordinates (little-endian)
    data[0] = x & 0xFF;
    data[1] = (x >> 8) & 0xFF;
    data[2] = y & 0xFF;
    data[3] = (y >> 8) & 0xFF;
    data[4] = color.r;
    data[5] = color.g;
    data[6] = color.b;

    return FPGA_SendCommand(CMD_DRAW_PIXEL, data, 7);
}

HAL_StatusTypeDef FPGA_DrawLine(Point2D start, Point2D end, Color color) {
    uint8_t data[11];

    // Pack start point
    data[0] = start.x & 0xFF;
    data[1] = (start.x >> 8) & 0xFF;
    data[2] = start.y & 0xFF;
    data[3] = (start.y >> 8) & 0xFF;

    // Pack end point
    data[4] = end.x & 0xFF;
    data[5] = (end.x >> 8) & 0xFF;
    data[6] = end.y & 0xFF;
    data[7] = (end.y >> 8) & 0xFF;

    // Pack color
    data[8] = color.r;
    data[9] = color.g;
    data[10] = color.b;

    return FPGA_SendCommand(CMD_DRAW_LINE, data, 11);
}

HAL_StatusTypeDef FPGA_DrawRect(Point2D topLeft, uint16_t width, uint16_t height, Color color, uint8_t filled) {
    uint8_t data[12];

    data[0] = topLeft.x & 0xFF;
    data[1] = (topLeft.x >> 8) & 0xFF;
    data[2] = topLeft.y & 0xFF;
    data[3] = (topLeft.y >> 8) & 0xFF;
    data[4] = width & 0xFF;
    data[5] = (width >> 8) & 0xFF;
    data[6] = height & 0xFF;
    data[7] = (height >> 8) & 0xFF;
    data[8] = color.r;
    data[9] = color.g;
    data[10] = color.b;
    data[11] = filled;

    return FPGA_SendCommand(CMD_DRAW_RECT, data, 12);
}

HAL_StatusTypeDef FPGA_DrawTriangle(Point2D p1, Point2D p2, Point2D p3, Color color, uint8_t filled) {
    uint8_t data[16];

    // Pack three points
    data[0] = p1.x & 0xFF;
    data[1] = (p1.x >> 8) & 0xFF;
    data[2] = p1.y & 0xFF;
    data[3] = (p1.y >> 8) & 0xFF;
    data[4] = p2.x & 0xFF;
    data[5] = (p2.x >> 8) & 0xFF;
    data[6] = p2.y & 0xFF;
    data[7] = (p2.y >> 8) & 0xFF;
    data[8] = p3.x & 0xFF;
    data[9] = (p3.x >> 8) & 0xFF;
    data[10] = p3.y & 0xFF;
    data[11] = (p3.y >> 8) & 0xFF;

    // Pack color and fill flag
    data[12] = color.r;
    data[13] = color.g;
    data[14] = color.b;
    data[15] = filled;

    return FPGA_SendCommand(CMD_DRAW_TRIANGLE, data, 16);
}

HAL_StatusTypeDef FPGA_DrawCube(Point3D center, uint16_t size, Color color) {
    uint8_t data[11];

    // Pack 3D center point
    data[0] = center.x & 0xFF;
    data[1] = (center.x >> 8) & 0xFF;
    data[2] = center.y & 0xFF;
    data[3] = (center.y >> 8) & 0xFF;
    data[4] = center.z & 0xFF;
    data[5] = (center.z >> 8) & 0xFF;

    // Pack size
    data[6] = size & 0xFF;
    data[7] = (size >> 8) & 0xFF;

    // Pack color
    data[8] = color.r;
    data[9] = color.g;
    data[10] = color.b;

    return FPGA_SendCommand(CMD_DRAW_CUBE, data, 11);
}

HAL_StatusTypeDef FPGA_SetCamera(Camera* cam) {
    uint8_t data[14];

    // Pack camera position
    data[0] = cam->position.x & 0xFF;
    data[1] = (cam->position.x >> 8) & 0xFF;
    data[2] = cam->position.y & 0xFF;
    data[3] = (cam->position.y >> 8) & 0xFF;
    data[4] = cam->position.z & 0xFF;
    data[5] = (cam->position.z >> 8) & 0xFF;

    // Pack camera rotation
    data[6] = cam->rotation.x & 0xFF;
    data[7] = (cam->rotation.x >> 8) & 0xFF;
    data[8] = cam->rotation.y & 0xFF;
    data[9] = (cam->rotation.y >> 8) & 0xFF;
    data[10] = cam->rotation.z & 0xFF;
    data[11] = (cam->rotation.z >> 8) & 0xFF;

    // Pack FOV
    data[12] = cam->fov & 0xFF;
    data[13] = (cam->fov >> 8) & 0xFF;

    return FPGA_SendCommand(CMD_SET_CAMERA, data, 14);
}

HAL_StatusTypeDef FPGA_SwapBuffers(void) {
    return FPGA_SendCommand(CMD_SWAP_BUFFER, NULL, 0);
}

HAL_StatusTypeDef FPGA_GetStatus(FPGA_Status* status) {
    uint8_t buffer[5];
    HAL_StatusTypeDef result = FPGA_ReadData(CMD_READ_STATUS, buffer, 5);

    if(result == HAL_OK) {
        status->ready = buffer[0];
        status->buffer_full = buffer[1];
        status->error_code = buffer[2];
        status->fps = buffer[3] | (buffer[4] << 8);
    }

    return result;
}

HAL_StatusTypeDef FPGA_Reset(void) {
    return FPGA_SendCommand(CMD_RESET, NULL, 0);
}
