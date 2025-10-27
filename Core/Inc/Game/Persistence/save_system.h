#ifndef INC_GAME_PERSISTENCE_SAVE_SYSTEM_H_
#define INC_GAME_PERSISTENCE_SAVE_SYSTEM_H_

#include "../game_types.h"
#include "stm32u5xx_hal.h"

// Save system initialization
void SaveSystem_Init(SPI_HandleTypeDef* hspi_sdcard);

// Save/Load operations
void SaveSystem_SaveStats(void);
void SaveSystem_LoadStats(void);

// Statistics management
void SaveSystem_RecordGame(uint32_t score, uint32_t time_played);
uint32_t SaveSystem_GetHighScore(void);
uint8_t SaveSystem_IsNewHighScore(uint32_t score);
GameStats* SaveSystem_GetStats(void);

#endif /* INC_GAME_PERSISTENCE_SAVE_SYSTEM_H_ */
