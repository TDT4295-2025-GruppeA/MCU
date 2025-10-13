#include "./Test/command_handler.h"
#include "./Game/game.h"
#include "./SDCard/sd_card.h"
#include "./SDCard/game_storage.h"
#include "./Game/shapes.h"
#include <string.h>

// External references
extern UART_HandleTypeDef huart1;
extern SPI_HandleTypeDef hspi3;
extern void UART_Printf(const char* format, ...);
extern uint8_t uart_rx;  // This needs to be in main.c

// Test functions (only if tests are compiled)
__attribute__((weak)) void Run_Collision_Tests(void) {
    UART_Printf("Collision tests not compiled. Build with RUN_UNIT_TESTS\r\n");
}

__attribute__((weak)) void Run_Obstacle_Tests(void) {
    UART_Printf("Obstacle tests not compiled. Build with RUN_UNIT_TESTS\r\n");
}

__attribute__((weak)) void Run_SDCard_Tests(void) {
    UART_Printf("SD tests not compiled. Build with RUN_UNIT_TESTS\r\n");
}

// Command buffer
char uart_command[64];
uint8_t uart_cmd_index = 0;

// Initialize command handler
void Command_Handler_Init(void) {
    memset(uart_command, 0, sizeof(uart_command));
    uart_cmd_index = 0;
    UART_Printf("\r\nCommand interface ready. Type 'help' for commands.\r\n> ");
}

// Process UART commands
void Process_UART_Command(const char* cmd) {
    UART_Printf("\r\n");

    // Test commands
    if(strcmp(cmd, "test") == 0) {
        UART_Printf("Available tests:\r\n");
        UART_Printf("  test all      - Run all tests\r\n");
        UART_Printf("  test sd       - Run SD card tests\r\n");
        UART_Printf("  test collision- Run collision tests\r\n");
        UART_Printf("  test obstacle - Run obstacle tests\r\n");
        UART_Printf("  test quick    - Quick SD verify\r\n");
    }
    else if(strcmp(cmd, "test all") == 0) {
        Run_All_Tests();
    }
    else if(strcmp(cmd, "test sd") == 0) {
        if(SD_IsPresent()) {
            Run_SDCard_Tests();
        } else {
            UART_Printf("SD card not present!\r\n");
        }
    }
    else if(strcmp(cmd, "test collision") == 0) {
        Run_Collision_Tests();
    }
    else if(strcmp(cmd, "test obstacle") == 0) {
        Run_Obstacle_Tests();
    }
    else if(strcmp(cmd, "test quick") == 0) {
        Quick_SD_Test();
    }

    // Info commands
    else if(strcmp(cmd, "stats") == 0) {
        Show_Game_Stats();
    }
    else if(strcmp(cmd, "sd info") == 0) {
        Show_SD_Info();
    }

    // Game commands
    else if(strcmp(cmd, "pause") == 0) {
        Game_Pause();
        UART_Printf("Game paused\r\n");
    }
    else if(strcmp(cmd, "resume") == 0) {
        Game_Resume();
        UART_Printf("Game resumed\r\n");
    }
    else if(strcmp(cmd, "reset") == 0) {
        Game_Reset();
        UART_Printf("Game reset\r\n");
    }

    // Help
    else if(strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        UART_Printf("=== Commands ===\r\n");
        UART_Printf("Tests:\r\n");
        UART_Printf("  test         - Show test menu\r\n");
        UART_Printf("  test all     - Run all tests\r\n");
        UART_Printf("  test sd      - Test SD card\r\n");
        UART_Printf("  test quick   - Quick SD test\r\n");
        UART_Printf("\r\nInfo:\r\n");
        UART_Printf("  stats        - Game statistics\r\n");
        UART_Printf("  sd info      - SD card info\r\n");
        UART_Printf("\r\nGame:\r\n");
        UART_Printf("  pause        - Pause game\r\n");
        UART_Printf("  resume       - Resume game\r\n");
        UART_Printf("  reset        - Reset game\r\n");
    }
    else if(strlen(cmd) > 0) {
        UART_Printf("Unknown: '%s'. Type 'help'\r\n", cmd);
    }

    UART_Printf("> ");
}

// Implementation of test/info functions
void Quick_SD_Test(void) {
    UART_Printf("Quick SD Test...\r\n");
    if(!SD_IsPresent()) {
        UART_Printf("  No SD card!\r\n");
        return;
    }

    uint8_t test[512];
    memset(test, 0x55, 512);

    if(SD_WriteBlock(999, test) == SD_OK) {
        UART_Printf("  Write: OK\r\n");
    }

    memset(test, 0, 512);
    if(SD_ReadBlock(999, test) == SD_OK && test[0] == 0x55) {
        UART_Printf("  Read: OK\r\n");
    }

    GameSave save;
    if(Storage_LoadGame(&save) == SD_OK) {
        UART_Printf("  High Score: %lu\r\n", save.high_score);
    }

    UART_Printf("Quick test complete!\r\n");
}

void Show_Game_Stats(void) {
    GameSave save;
    if(Storage_LoadGame(&save) == SD_OK) {
        UART_Printf("Game Statistics:\r\n");
        UART_Printf("  High Score: %lu\r\n", save.high_score);
        UART_Printf("  Games Played: %lu\r\n", save.total_played);
        UART_Printf("  Total Time: %lu sec\r\n", save.total_time);
    } else {
        UART_Printf("No stats available\r\n");
    }
}

void Show_SD_Info(void) {
    if(!SD_IsPresent()) {
        UART_Printf("No SD card!\r\n");
        return;
    }

    SDCardInfo info;
    SD_GetCardInfo(&info);

    UART_Printf("SD Card Info:\r\n");
    UART_Printf("  Type: %s\r\n",
                info.card_type == 2 ? "SDHC" :
                info.card_type == 1 ? "SD" : "MMC");
    UART_Printf("  Block Size: %lu\r\n", info.block_size);
    UART_Printf("  Shapes stored: %s\r\n",
                Storage_ShapeExists(SHAPE_PLAYER) ? "Yes" : "No");
}

void Run_All_Tests(void) {
    UART_Printf("=== RUNNING ALL TESTS ===\r\n");

    Run_Collision_Tests();
    Run_Obstacle_Tests();

    if(SD_IsPresent()) {
        Run_SDCard_Tests();
    } else {
        UART_Printf("SD not present - skipping SD tests\r\n");
    }

    UART_Printf("=== TESTS COMPLETE ===\r\n");
}
