// spi_protocol.c - SPI communication implementation
#include "./Game/spi_protocol.h"
#include <string.h>

// External UART for debugging
extern void UART_Printf(const char* format, ...);

// Optional: forward raw SPI packets to the PC over USART (ST-LINK VCP) for simulator
#ifdef SIM_UART
#include "main.h"
extern UART_HandleTypeDef huart1;
// Mirrors SPI packets to UART (e.g., ST-LINK VCP) for use by the simulator or other external tools.
// This is required for the PC simulator to receive identical bytes as the FPGA, but can be used for any interface needing SPI traffic.
static void SPI_MirrorPacketToUART(uint8_t* data, uint16_t size)
{
    if (huart1.Instance != NULL)
    {
        const char* prefix = "Sending SPI message: ";
        HAL_UART_Transmit(&huart1, (uint8_t*)prefix, strlen(prefix), 100);
        HAL_UART_Transmit(&huart1, data, size, 100);
        const char* suffix = "SPI message end";
        HAL_UART_Transmit(&huart1, (uint8_t*)suffix, strlen(suffix), 100);
    }
}
#endif

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


void SPI_TransmitPacket(uint8_t* data, uint16_t size)
{
    if(hspi != NULL) {
        // Pull CS low
        HAL_GPIO_WritePin(SPI_CS_PORT, SPI_CS_PIN, GPIO_PIN_RESET);
        // Transmit data
        HAL_SPI_Transmit(hspi, data, size, 100);
        // Pull CS high
        HAL_GPIO_WritePin(SPI_CS_PORT, SPI_CS_PIN, GPIO_PIN_SET);
    }
    #ifdef SIM_UART
    // Mirror the same bytes to UART for simulator or other external interfaces
    SPI_MirrorPacketToUART(data, size);
    #endif
}

// --- Helpers ---
static int32_t to_q16_16(float v) { return (int32_t)(v * 65536.0f); }
static void pack_be32(uint8_t *buf, int32_t val) {
    buf[0] = (uint8_t)((val >> 24) & 0xFF);
    buf[1] = (uint8_t)((val >> 16) & 0xFF);
    buf[2] = (uint8_t)((val >> 8) & 0xFF);
    buf[3] = (uint8_t)(val & 0xFF);
}
static void pack_be16(uint8_t *buf, uint16_t val) {
    buf[0] = (uint8_t)((val >> 8) & 0xFF);
    buf[1] = (uint8_t)(val & 0xFF);
}

// --- Protocol commands ---
// Reset (0x00)
void SPI_SendReset(void)
{
    uint8_t cmd = 0x00;
    SPI_TransmitPacket(&cmd, 1);
}

// Begin upload (0xA0) -> FPGA may respond with 1-byte object ID during/after transaction
// This function attempts to read a single byte response if the SPI backend is available.
// Returns 0 if no response is available or on timeout.
uint8_t SPI_BeginUpload(void)
{
    uint8_t cmd = 0xA0;
    uint8_t resp = 0;

    if(hspi == NULL) {
        #ifdef SIM_UART
        SPI_TransmitPacket(&cmd, 1);
        #endif
        return 0;
    }

    HAL_GPIO_WritePin(SPI_CS_PORT, SPI_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(hspi, &cmd, 1, 100);
    // Try to receive 1 byte response (non-blocking with timeout)
    if(HAL_SPI_Receive(hspi, &resp, 1, 50) != HAL_OK) {
        resp = 0;
    }
    HAL_GPIO_WritePin(SPI_CS_PORT, SPI_CS_PIN, GPIO_PIN_SET);

    #ifdef SIM_UART
    SPI_MirrorPacketToUART(&cmd, 1);
    if(resp != 0) {
        SPI_MirrorPacketToUART(&resp, 1);
    }
    #endif

    return resp;
}

// Upload triangle (0xA1): Color (2 bytes), V0 (3x Q16.16), V1 (3x Q16.16), V2 (3x Q16.16)
void SPI_UploadTriangle(uint16_t color, float v0[3], float v1[3], float v2[3])
{
    uint8_t packet[41]; // 1 + 2 + 12 + 12 + 12 = 41 bytes
    uint16_t idx = 0;
    packet[idx++] = 0xA1;
    pack_be16(&packet[idx], color); idx += 2;

    int32_t tmp;
    tmp = to_q16_16(v0[0]); pack_be32(&packet[idx], tmp); idx += 4;
    tmp = to_q16_16(v0[1]); pack_be32(&packet[idx], tmp); idx += 4;
    tmp = to_q16_16(v0[2]); pack_be32(&packet[idx], tmp); idx += 4;

    tmp = to_q16_16(v1[0]); pack_be32(&packet[idx], tmp); idx += 4;
    tmp = to_q16_16(v1[1]); pack_be32(&packet[idx], tmp); idx += 4;
    tmp = to_q16_16(v1[2]); pack_be32(&packet[idx], tmp); idx += 4;

    tmp = to_q16_16(v2[0]); pack_be32(&packet[idx], tmp); idx += 4;
    tmp = to_q16_16(v2[1]); pack_be32(&packet[idx], tmp); idx += 4;
    tmp = to_q16_16(v2[2]); pack_be32(&packet[idx], tmp); idx += 4;

    SPI_TransmitPacket(packet, 41);
}

// Add model instance (0xB0): Reserved(1), Model ID(1), Position X/Y/Z (Q16.16), Rotation 3x3 (Q16.16)
void SPI_AddModelInstance(uint8_t model_id, float pos[3], float rot[9])
{
    /* packet layout:
     * 1 byte opcode
     * 1 byte reserved
     * 1 byte model_id
     * 3 * 4 bytes position (Q16.16)
     * 9 * 4 bytes rotation (Q16.16)
     * = 1 + 1 + 1 + 12 + 36 = 51 bytes
     */
    uint8_t packet[51];
    uint16_t idx = 0;
    packet[idx++] = 0xB0;
    packet[idx++] = 0x00; // reserved
    packet[idx++] = model_id;

    pack_be32(&packet[idx], to_q16_16(pos[0])); idx += 4;
    pack_be32(&packet[idx], to_q16_16(pos[1])); idx += 4;
    pack_be32(&packet[idx], to_q16_16(pos[2])); idx += 4;

    for(int i = 0; i < 9; ++i) {
        pack_be32(&packet[idx], to_q16_16(rot[i])); idx += 4;
    }

    SPI_TransmitPacket(packet, idx);
}

// Mark frame start (0xF0)
void SPI_MarkFrameStart(void)
{
    uint8_t cmd = 0xF0;
    SPI_TransmitPacket(&cmd, 1);
}

// Mark frame end (0xF1)
void SPI_MarkFrameEnd(void)
{
    uint8_t cmd = 0xF1;
    SPI_TransmitPacket(&cmd, 1);
}

