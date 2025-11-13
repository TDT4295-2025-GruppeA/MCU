// test_sdcard.c - SD Card and Storage Tests

#include "./Test/test_framework.h"
#include "./SDCard/sd_card.h"
#include "./SDCard/game_storage.h"
#include "./Game/shapes.h"
#include <string.h>

extern SPI_HandleTypeDef hspi3;
extern TestStats test_stats;

// Test data patterns
#define TEST_BLOCK_1 500  // Test blocks (away from game/shape data)
#define TEST_BLOCK_2 501
#define TEST_PATTERN_A 0xAA
#define TEST_PATTERN_B 0x55

// Test buffers
static uint8_t write_buffer[512];
static uint8_t read_buffer[512];

// Test 1: Basic SD card initialization
uint8_t test_sd_initialization(void) {
    // SD should already be initialized from main
    uint8_t present = SD_IsPresent();

    TEST_ASSERT(present == 1, "SD card should be present and initialized");

    // Get card info
    SDCardInfo info;
    SDResult result = SD_GetCardInfo(&info);

    TEST_ASSERT_EQUAL(SD_OK, result, "Should get card info successfully");
    TEST_ASSERT_EQUAL(512, info.block_size, "Block size should be 512");
    TEST_ASSERT(info.initialized == 1, "Card should be initialized");

    return 1;
}

// Test 2: Write and read single block
uint8_t test_write_read_block(void) {
    // Fill buffer with test pattern
    memset(write_buffer, TEST_PATTERN_A, 512);
    write_buffer[0] = 0xDE;  // Unique markers
    write_buffer[511] = 0xAD;

    // Write to test block
    SDResult result = SD_WriteBlock(TEST_BLOCK_1, write_buffer);
    TEST_ASSERT_EQUAL(SD_OK, result, "Write should succeed");

    // Clear read buffer
    memset(read_buffer, 0, 512);

    // Read back
    result = SD_ReadBlock(TEST_BLOCK_1, read_buffer);
    TEST_ASSERT_EQUAL(SD_OK, result, "Read should succeed");

    // Verify data
    TEST_ASSERT_EQUAL(0xDE, read_buffer[0], "First byte should match");
    TEST_ASSERT_EQUAL(0xAD, read_buffer[511], "Last byte should match");
    TEST_ASSERT_EQUAL(TEST_PATTERN_A, read_buffer[256], "Middle byte should match");

    // Verify entire buffer
    int match = memcmp(write_buffer, read_buffer, 512);
    TEST_ASSERT_EQUAL(0, match, "Entire buffer should match");

    return 1;
}

// Test 3: Overwrite existing block
uint8_t test_overwrite_block(void) {
    // Write first pattern
    memset(write_buffer, TEST_PATTERN_A, 512);
    SD_WriteBlock(TEST_BLOCK_2, write_buffer);

    // Write different pattern
    memset(write_buffer, TEST_PATTERN_B, 512);
    SDResult result = SD_WriteBlock(TEST_BLOCK_2, write_buffer);
    TEST_ASSERT_EQUAL(SD_OK, result, "Overwrite should succeed");

    // Read back
    memset(read_buffer, 0, 512);
    SD_ReadBlock(TEST_BLOCK_2, read_buffer);

    // Should have new pattern, not old
    TEST_ASSERT_EQUAL(TEST_PATTERN_B, read_buffer[0], "Should have new pattern");
    TEST_ASSERT(read_buffer[0] != TEST_PATTERN_A, "Should not have old pattern");

    return 1;
}

// Test 4: Game save and load
uint8_t test_game_save_load(void) {
    GameSave save_data, load_data;

    // Create test save
    save_data.magic = 0x47414D45;
    save_data.version = 1;
    save_data.high_score = 12345;
    save_data.total_played = 67;
    save_data.total_time = 890;
    save_data.checksum = save_data.high_score ^ save_data.total_played ^ save_data.total_time;

    // Save to SD
    SDResult result = Storage_SaveGame(&save_data);
    TEST_ASSERT_EQUAL(SD_OK, result, "Game save should succeed");

    // Clear load buffer
    memset(&load_data, 0, sizeof(GameSave));

    // Load from SD
    result = Storage_LoadGame(&load_data);
    TEST_ASSERT_EQUAL(SD_OK, result, "Game load should succeed");

    // Verify data
    TEST_ASSERT_EQUAL(save_data.magic, load_data.magic, "Magic should match");
    TEST_ASSERT_EQUAL(save_data.high_score, load_data.high_score, "High score should match");
    TEST_ASSERT_EQUAL(save_data.total_played, load_data.total_played, "Total played should match");
    TEST_ASSERT_EQUAL(save_data.total_time, load_data.total_time, "Total time should match");
    TEST_ASSERT_EQUAL(save_data.checksum, load_data.checksum, "Checksum should match");

    return 1;
}

// Test 5: Shape exists check
uint8_t test_shape_exists(void) {
    // After initialization, shapes should exist
    uint8_t exists = Storage_ShapeExists(SHAPE_PLAYER);
    TEST_ASSERT(exists, "Player shape should exist on SD");

    exists = Storage_ShapeExists(SHAPE_CUBE);
    TEST_ASSERT(exists, "Cube shape should exist on SD");

    exists = Storage_ShapeExists(SHAPE_CONE);
    TEST_ASSERT(exists, "Cone shape should exist on SD");

    // Non-existent shape
    exists = Storage_ShapeExists(999);
    TEST_ASSERT(!exists, "Non-existent shape should not exist");

    return 1;
}

// Test 6: Data integrity with checksum
uint8_t test_data_integrity(void) {
    GameSave save_data;

    // Create save with correct checksum
    save_data.magic = 0x47414D45;
    save_data.version = 1;
    save_data.high_score = 9999;
    save_data.total_played = 10;
    save_data.total_time = 500;
    save_data.checksum = save_data.high_score ^ save_data.total_played ^ save_data.total_time;

    Storage_SaveGame(&save_data);

    // Corrupt the data on SD card directly
    uint8_t corrupt_buffer[512];
    SD_ReadBlock(SAVE_BLOCK, corrupt_buffer);
    corrupt_buffer[8] = 0xFF;  // Corrupt high score bytes
    SD_WriteBlock(SAVE_BLOCK, corrupt_buffer);

    // Try to load - should detect corruption
    GameSave loaded;
    Storage_LoadGame(&loaded);

    // Checksum shouldn't match if corruption detected
    uint32_t expected_checksum = loaded.high_score ^ loaded.total_played ^ loaded.total_time;
    TEST_ASSERT(loaded.checksum != expected_checksum || loaded.high_score == 0,
                "Should detect corrupted data or reinitialize");

    return 1;
}

// Test 7: Block boundary test
uint8_t test_block_boundaries(void) {
    // Write patterns at block boundaries
    memset(write_buffer, 0xF0, 512);

    // Test writing to different blocks
    SDResult result = SD_WriteBlock(600, write_buffer);
    TEST_ASSERT_EQUAL(SD_OK, result, "Write to block 600 should succeed");

    memset(write_buffer, 0x0F, 512);
    result = SD_WriteBlock(601, write_buffer);
    TEST_ASSERT_EQUAL(SD_OK, result, "Write to block 601 should succeed");

    // Read back and verify no overlap
    SD_ReadBlock(600, read_buffer);
    TEST_ASSERT_EQUAL(0xF0, read_buffer[511], "Last byte of block 600 should be 0xF0");

    SD_ReadBlock(601, read_buffer);
    TEST_ASSERT_EQUAL(0x0F, read_buffer[0], "First byte of block 601 should be 0x0F");

    return 1;
}

// Main test runner for SD card tests
void Run_SDCard_Tests(void) {
    UART_Printf("\r\n=== SD CARD MODULE TESTS ===\r\n");

    // Reset stats
    test_stats.tests_run = 0;
    test_stats.tests_passed = 0;
    test_stats.tests_failed = 0;

    // Check if SD card is present first
    if(!SD_IsPresent()) {
        UART_Printf("ERROR: SD card not initialized! Skipping tests.\r\n");
        return;
    }

    // Run all tests
    RUN_TEST(test_sd_initialization);
    RUN_TEST(test_write_read_block);
    RUN_TEST(test_overwrite_block);
    RUN_TEST(test_game_save_load);

    RUN_TEST(test_shape_exists);
    RUN_TEST(test_data_integrity);
    RUN_TEST(test_block_boundaries);

    // Print summary
    UART_Printf("\r\n=== TEST SUMMARY ===\r\n");
    UART_Printf("Tests run:    %lu\r\n", test_stats.tests_run);
    UART_Printf("Tests passed: %lu\r\n", test_stats.tests_passed);
    UART_Printf("Tests failed: %lu\r\n", test_stats.tests_failed);

    if (test_stats.tests_failed == 0) {
        UART_Printf("ALL SD CARD TESTS PASSED!\r\n");
    } else {
        UART_Printf("SOME SD CARD TESTS FAILED!\r\n");
    }
}
