#include "./Game/spi_protocol.h"
#include "./Utilities/transform.h"
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
