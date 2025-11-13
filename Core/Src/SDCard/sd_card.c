#include "./SDCard/sd_card.h"
#include <string.h>

extern void UART_Printf(const char* format, ...);

static SPI_HandleTypeDef* sd_spi = NULL;
static SDCardInfo card_info = {0};
static uint8_t sd_type = 0;

// CS Pin control
static void SD_CS_Low(void) {
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_RESET);
}

static void SD_CS_High(void) {
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_SET);
}

// Send dummy byte to generate clock
static uint8_t SD_ReadByte(void) {
    uint8_t dummy = 0xFF;
    uint8_t response;
    HAL_SPI_TransmitReceive(sd_spi, &dummy, &response, 1, 100);
    return response;
}

// Send single byte
static void SD_WriteByte(uint8_t data) {
    HAL_SPI_Transmit(sd_spi, &data, 1, 100);
}

// Wait for card ready
static uint8_t SD_WaitReady(uint32_t timeout) {
    uint8_t response;
    uint32_t start = HAL_GetTick();

    do {
        response = SD_ReadByte();
        if(response == 0xFF) return 1;
    } while((HAL_GetTick() - start) < timeout);

    return 0;
}

// Send command to SD card
static uint8_t SD_SendCommand(uint8_t cmd, uint32_t arg) {
    uint8_t crc;
    uint8_t response;

    // Wait for card ready
    if(!SD_WaitReady(5000)) return 0xFF;

    // Send command packet
    SD_WriteByte(cmd);
    SD_WriteByte(arg >> 24);
    SD_WriteByte(arg >> 16);
    SD_WriteByte(arg >> 8);
    SD_WriteByte(arg);

    // CRC
    if(cmd == CMD0) crc = 0x95;      // CRC for CMD0
    else if(cmd == CMD8) crc = 0x87; // CRC for CMD8
    else crc = 0x01;                 // Dummy CRC

    SD_WriteByte(crc);

    // Skip stuff byte for stop read
    if(cmd == CMD12) SD_ReadByte();

    // Wait for response
    uint8_t retry = 10;
    do {
        response = SD_ReadByte();
    } while((response & 0x80) && retry--);

    return response;
}



// Initialize SD Card
SDResult SD_Init(SPI_HandleTypeDef* hspi) {
    sd_spi = hspi;
    uint8_t response;
    int retry;

    UART_Printf("SD: Initializing...\r\n");

    SD_CS_High();
    HAL_Delay(10);

    UART_Printf("SD: Power up sequence...\r\n");
    for(int i = 0; i < 10; i++) {
        SD_WriteByte(0xFF);
    }

    UART_Printf("SD: Sending CMD0...\r\n");

    // Try multiple times with proper timing
    for(int attempt = 0; attempt < 1; attempt++) {
        SD_CS_Low();
        HAL_Delay(1);  // Short delay after CS low

        // Send CMD0 with proper sequence
        response = SD_SendCommand(CMD0, 0);

        if(response == 0x01) {
            UART_Printf("SD: Card in idle state (attempt %d)!\r\n", attempt + 1);
            goto card_ready;
        }

        UART_Printf("  CMD0 attempt %d: response=0x%02X\r\n", attempt, response);

        SD_CS_High();
        HAL_Delay(100);
    }

    UART_Printf("SD: Failed to get idle state\r\n");
    SD_CS_High();
    return SD_NO_CARD;

card_ready:
    UART_Printf("SD: Checking card version...\r\n");
    response = SD_SendCommand(CMD8, 0x1AA);

    if(response == 0x01) {
        // SDv2 card
        UART_Printf("SD: SDv2 card detected\r\n");

        // Read R7 response
        uint8_t r7[4];
        for(int i = 0; i < 4; i++) {
            r7[i] = SD_ReadByte();
        }

        // Send ACMD41 for SDv2
        retry = 200;
        do {
            response = SD_SendCommand(CMD55, 0);
            if(response <= 0x01) {
                response = SD_SendCommand(ACMD41, 0x40000000);
                if(response == 0x00) {
                    UART_Printf("SD: Card initialized (SDv2)\r\n");
                    break;
                }
            }
            HAL_Delay(10);
        } while(--retry > 0);

        if(retry == 0) {
            UART_Printf("SD: ACMD41 timeout\r\n");
            SD_CS_High();
            return SD_ERROR;
        }

        // Check CCS bit for SDHC
        response = SD_SendCommand(CMD58, 0);
        if(response == 0x00) {
            uint8_t ocr[4];
            for(int i = 0; i < 4; i++) {
                ocr[i] = SD_ReadByte();
            }
            sd_type = (ocr[0] & 0x40) ? 2 : 1;  // Type 2 = SDHC
            UART_Printf("SD: Card type: %s\r\n", (sd_type == 2) ? "SDHC" : "SD");
        }

    } else if(response == 0x05) {
        // SDv1 or MMC card
        UART_Printf("SD: SDv1 or MMC card detected\r\n");

        // Try SDv1 first
        response = SD_SendCommand(CMD55, 0);
        response = SD_SendCommand(ACMD41, 0);

        if(response <= 0x01) {
            // SDv1 card
            sd_type = 1;
            retry = 200;
            do {
                response = SD_SendCommand(CMD55, 0);
                response = SD_SendCommand(ACMD41, 0);
                if(response == 0x00) {
                    UART_Printf("SD: Card initialized (SDv1)\r\n");
                    break;
                }
                HAL_Delay(10);
            } while(--retry > 0);

        } else {
            // Try MMC
            retry = 200;
            do {
                response = SD_SendCommand(CMD1, 0);
                if(response == 0x00) {
                    UART_Printf("SD: Card initialized (MMC)\r\n");
                    sd_type = 0;
                    break;
                }
                HAL_Delay(10);
            } while(--retry > 0);
        }

        if(retry == 0) {
            UART_Printf("SD: Init timeout\r\n");
            SD_CS_High();
            return SD_ERROR;
        }
    } else {
        UART_Printf("SD: Unknown card response to CMD8: 0x%02X\r\n", response);
        SD_CS_High();
        return SD_ERROR;
    }

    // Set block size to 512 bytes
    response = SD_SendCommand(CMD16, 512);
    if(response != 0x00) {
        UART_Printf("SD: Failed to set block size\r\n");
    }

    //Deselect and complete
    SD_CS_High();

    // Update card info
    card_info.initialized = 1;
    card_info.block_size = 512;
    card_info.card_type = sd_type;

    UART_Printf("SD: Init complete! Type=%d\r\n", sd_type);
    return SD_OK;
}



// Read single block
SDResult SD_ReadBlock(uint32_t block_addr, uint8_t* buffer) {
    if(!card_info.initialized) return SD_ERROR;

    // Convert to byte address if needed
    if(sd_type != 2) block_addr *= 512;

    SD_CS_Low();

    if(SD_SendCommand(CMD17, block_addr) != 0x00) {
        SD_CS_High();
        return SD_READ_ERROR;
    }

    // Wait for data token
    uint16_t retry = 40000;
    uint8_t token;
    do {
        token = SD_ReadByte();
    } while((token == 0xFF) && retry--);

    if(token != 0xFE) {
        SD_CS_High();
        return SD_READ_ERROR;
    }

    // Read data
    for(int i = 0; i < 512; i++) {
        buffer[i] = SD_ReadByte();
    }

    // Read CRC (ignored)
    SD_ReadByte();
    SD_ReadByte();

    SD_CS_High();
    return SD_OK;
}

// Write single block
SDResult SD_WriteBlock(uint32_t block_addr, const uint8_t* data) {
    if(!card_info.initialized) return SD_ERROR;

    // Convert to byte address if needed
    if(sd_type != 2) block_addr *= 512;

    SD_CS_Low();

    if(SD_SendCommand(CMD24, block_addr) != 0x00) {
        SD_CS_High();
        return SD_WRITE_ERROR;
    }

    // Send data token
    SD_WriteByte(0xFE);

    // Write data
    for(int i = 0; i < 512; i++) {
        SD_WriteByte(data[i]);
    }

    // Send dummy CRC
    SD_WriteByte(0xFF);
    SD_WriteByte(0xFF);

    // Check response
    uint8_t response = SD_ReadByte();
    if((response & 0x1F) != 0x05) {
        SD_CS_High();
        return SD_WRITE_ERROR;
    }

    // Wait for card to complete write
    if(!SD_WaitReady(500)) {
        SD_CS_High();
        return SD_TIMEOUT;
    }

    SD_CS_High();
    return SD_OK;
}


// Check if card is present
uint8_t SD_IsPresent(void) {
    return card_info.initialized;
}

// Get card info
SDResult SD_GetCardInfo(SDCardInfo* info) {
    if(!card_info.initialized) return SD_ERROR;

    memcpy(info, &card_info, sizeof(SDCardInfo));
    return SD_OK;
}
