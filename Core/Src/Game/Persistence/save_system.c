#include "../../../Inc/Game/Persistence/save_system.h"
#include "../../../Inc/SDCard/sd_card.h"
#include "../../../Inc/SDCard/game_storage.h"

extern void UART_Printf(const char* format, ...);

static GameStats game_stats = {0};
static uint8_t sd_card_ready = 0;
static SPI_HandleTypeDef* hspi_sd = NULL;

void SaveSystem_Init(SPI_HandleTypeDef* hspi_sdcard)
{
    hspi_sd = hspi_sdcard;

    UART_Printf("Initializing SD Card...\r\n");
    if(Storage_Init(hspi_sd) == SD_OK) {
        sd_card_ready = 1;
        Storage_InitializeShapes();
        UART_Printf("SD Card ready\r\n");
    } else {
        sd_card_ready = 0;
        UART_Printf("SD Card not found - playing without saves\r\n");
    }
}

void SaveSystem_SaveStats(void)
{
    if(!sd_card_ready) return;

    GameSave save;
    save.magic = 0x47414D45;  // "GAME"
    save.version = 1;
    save.high_score = game_stats.high_score;
    save.total_played = game_stats.total_games;
    save.total_time = game_stats.total_time;

    // Simple checksum
    save.checksum = save.high_score ^ save.total_played ^ save.total_time;

    if(Storage_SaveGame(&save) == SD_OK) {
        UART_Printf("Stats saved to SD card\r\n");
    } else {
        UART_Printf("Failed to save stats\r\n");
    }
}

void SaveSystem_LoadStats(void)
{
    if(!sd_card_ready) return;

    GameSave save;
    if(Storage_LoadGame(&save) == SD_OK) {
        // Verify checksum
        uint32_t check = save.high_score ^ save.total_played ^ save.total_time;
        if(check == save.checksum) {
            game_stats.high_score = save.high_score;
            game_stats.total_games = save.total_played;
            game_stats.total_time = save.total_time;
            UART_Printf("Stats loaded. High Score: %lu\r\n", game_stats.high_score);
        } else {
            UART_Printf("Save file corrupted, starting fresh\r\n");
        }
    } else {
        UART_Printf("No save file found, creating new\r\n");
    }
}

void SaveSystem_RecordGame(uint32_t score, uint32_t time_played)
{
    game_stats.total_games++;
    game_stats.total_time += time_played;
    game_stats.last_score = score;

    if(score > game_stats.high_score) {
        game_stats.high_score = score;
    }

    SaveSystem_SaveStats();
}

uint32_t SaveSystem_GetHighScore(void)
{
    return game_stats.high_score;
}

uint8_t SaveSystem_IsNewHighScore(uint32_t score)
{
    return (score > game_stats.high_score);
}

GameStats* SaveSystem_GetStats(void)
{
    return &game_stats;
}



