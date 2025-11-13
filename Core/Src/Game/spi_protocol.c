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
        // Send null byte due to error on FPGA side
        // (They're idiots)
        uint8_t dummy_data = 0;
        HAL_SPI_Transmit(hspi, &dummy_data, 1, 10);
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
    uint8_t begin_packet[2];
    begin_packet[0] = CMD_BEGIN_UPLOAD;
    begin_packet[1] = model_id;
    SPI_TransmitPacket(begin_packet, 2);

    // Upload triangles
    for(int i = 0; i < shape->triangle_count; i++) {
        uint8_t packet[43];
        packet[0] = CMD_UPLOAD_TRIANGLE;

        uint16_t colors[] = {0xFFFF, 0xFF00, 0x00FF};

        for(int v = 0; v < 3; v++) {
            uint8_t vertex_idx = (v == 0) ? shape->triangles[i].v1 :
                                (v == 1) ? shape->triangles[i].v2 :
                                          shape->triangles[i].v3;

            int32_t x = (int32_t)(shape->vertices[vertex_idx].x * 65536.0f);
            int32_t y = (int32_t)(shape->vertices[vertex_idx].y * 65536.0f);
            int32_t z = (int32_t)(shape->vertices[vertex_idx].z * 65536.0f);

            int offset = 1 + (v * 14);
            // Pack all three coordinates
            packet[offset++] = colors[v] >> 4;  // RGB byte 1
            packet[offset++] = colors[v];  // RGB byte 2
            packet[offset++] = (x >> 24) & 0xFF;
            packet[offset++] = (x >> 16) & 0xFF;
            packet[offset++] = (x >> 8) & 0xFF;
            packet[offset++] = x & 0xFF;

            packet[offset++] = (y >> 24) & 0xFF;
            packet[offset++] = (y >> 16) & 0xFF;
            packet[offset++] = (y >> 8) & 0xFF;
            packet[offset++] = y & 0xFF;

            packet[offset++] = (z >> 24) & 0xFF;
            packet[offset++] = (z >> 16) & 0xFF;
            packet[offset++] = (z >> 8) & 0xFF;
            packet[offset++] = z & 0xFF;
        }

        SPI_TransmitPacket(packet, 43);
    }

    UART_Printf("SPI: Uploaded model ID %d with %d triangles\r\n",
                model_id, shape->triangle_count);
}

void SPI_AddModelInstance(uint8_t shape_id, Position* pos, float* rotation_matrix, uint8_t is_last_model)
{
    uint8_t packet[51];
    memset(packet, 0, 51);

    packet[0] = CMD_ADD_INSTANCE;
    packet[1] = is_last_model ? 0x01 : 0x00;  // Last model flag
    packet[2] = shape_id;

    // Position in fixed-point
    int32_t x_fixed = (int32_t)(pos->x * 65536.0f);
    int32_t y_fixed = (int32_t)(pos->y * 65536.0f);
    int32_t z_fixed = (int32_t)(pos->z * 65536.0f);

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

    SPI_TransmitPacket((uint8_t*)packet, 51);
}
