#ifndef INC_SDCARD_SD_CARD_H_
#define INC_SDCARD_SD_CARD_H_

#include "main.h"
#include <stdint.h>

// SD Card Commands
#define CMD0    0x40  // GO_IDLE_STATE
#define CMD1    0x41  // SEND_OP_COND
#define CMD8    0x48  // SEND_IF_COND
#define CMD9    0x49  // SEND_CSD
#define CMD10   0x4A  // SEND_CID
#define CMD12   0x4C  // STOP_TRANSMISSION
#define CMD16   0x50  // SET_BLOCKLEN
#define CMD17   0x51  // READ_SINGLE_BLOCK
#define CMD18   0x52  // READ_MULTIPLE_BLOCK
#define CMD24   0x58  // WRITE_BLOCK
#define CMD25   0x59  // WRITE_MULTIPLE_BLOCK
#define CMD55   0x77  // APP_CMD
#define CMD58   0x7A  // READ_OCR
#define ACMD23  0x57  // SET_WR_BLK_ERASE_COUNT
#define ACMD41  0x69  // SD_SEND_OP_COND

// SD Card Responses
#define SD_READY        0x00
#define SD_IDLE_STATE   0x01
#define SD_SUCCESS      0x00

// SD Card CS Pin
#define SD_CS_PORT  GPIOB
#define SD_CS_PIN   GPIO_PIN_0

// Result codes
typedef enum {
    SD_OK = 0,
    SD_ERROR,
    SD_TIMEOUT,
    SD_NO_CARD,
    SD_WRITE_ERROR,
    SD_READ_ERROR
} SDResult;

// SD Card info structure
typedef struct {
    uint32_t capacity;      // Card capacity in bytes
    uint32_t block_size;    // Block size
    uint8_t  card_type;     // Card type (SD, SDHC, etc.)
    uint8_t  initialized;   // Init status
} SDCardInfo;

// Function prototypes
SDResult SD_Init(SPI_HandleTypeDef* hspi);
SDResult SD_ReadBlock(uint32_t block_addr, uint8_t* buffer);
SDResult SD_WriteBlock(uint32_t block_addr, const uint8_t* data);
SDResult SD_ReadMultipleBlocks(uint32_t block_addr, uint8_t* buffer, uint32_t count);
SDResult SD_WriteMultipleBlocks(uint32_t block_addr, const uint8_t* data, uint32_t count);
SDResult SD_GetCardInfo(SDCardInfo* info);
uint8_t  SD_IsPresent(void);



#endif /* INC_SDCARD_SD_CARD_H_ */
