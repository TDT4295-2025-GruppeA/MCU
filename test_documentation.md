# Test Documentation - Flyboy 3D

## Overview

This project implements a comprehensive testing framework for the Flyboy 3D game, ensuring reliability across collision detection, obstacle management, SD card operations, and shape storage systems.

## Test Framework

### Architecture

The test framework uses a lightweight macro-based approach optimized for embedded systems:

```c
// Test assertion macros (test_framework.h)
TEST_ASSERT(condition, message)           // Basic assertion
TEST_ASSERT_EQUAL(expected, actual, msg)  // Equality check
TEST_ASSERT_FLOAT_EQUAL(exp, act, tol, msg) // Float comparison
RUN_TEST(test_function)                   // Test runner
```

### Test Statistics

Each test suite tracks:
- Tests run
- Tests passed  
- Tests failed
- Execution time (for performance tests)

## Test Suites

### 1. Collision Detection Tests (`test_collision.c`)

**Coverage**: 6 tests, 100% branch coverage

#### Tests:
- `test_box_intersection_overlap`: Verifies AABB collision detection
- `test_box_intersection_no_overlap`: Ensures no false positives
- `test_boundary_collision`: World boundary checking
- `test_player_obstacle_collision`: Player-obstacle interaction
- `test_no_collision`: Validates collision-free scenarios
- `test_exact_boundary`: Edge case testing

#### Key Validations:
```c
// Collision box calculation
Box1: Center(0,0,0), Size(10,10,10) → Bounds[-5 to 5]
Box2: Center(3,0,0), Size(10,10,10) → Bounds[-2 to 8]
Result: Overlapping (X axes intersect)
```

### 2. Obstacle Management Tests (`test_obstacles.c`)

**Coverage**: 7 tests, 90% code coverage

#### Tests:
- `test_obstacle_initialization`: Verifies clean startup state
- `test_obstacle_spawn`: Validates spawn mechanics
- `test_obstacle_despawn`: Validates despawn mechanics
- `test_auto_spawn_ahead`: Validates spawning in spesific pattern ahead of player
- `test_visible_count`: Validates only x-amount visible at given time
- `test_shape_variety`: Validates that different shapes get spawned
- `test_spawn_spacing`: Validates that spawning happenes within given spaces




#### Critical Constraints:
- Minimum spacing: 30 units (Z-axis)
- X boundaries: -40 to +40
- Maximum active: 10 obstacles
- Cleanup distance: 20 units behind player

### 3. SD Card Operations Tests (`test_sdcard.c`)

**Coverage**: 7 tests, SD card driver validation

#### Tests:
- `test_sd_initialization`: Card detection and setup
- `test_write_read_block`: Single block operations
- `test_overwrite_block`: Data overwrite verification
- `test_game_save_load`: High score persistence
- `test_shape_exists`: Shape presence detection
- `test_data_integrity`: Checksum validation
- `test_block_boundaries`: Multi-block boundary handling

#### Storage Map:
```
Block 100: Game saves (high score, statistics)
Block 200-202: Shape data (player, cube, cone)  
Block 500-999: Test data (non-destructive)
```

#### Performance Benchmarks:
- Write: 10 blocks < 5 seconds
- Read: 10 blocks < 2 seconds
- Shape load: < 50ms per shape

### 4. Shape Storage Tests (Integrated)

**Validation Points**:
- Vertex count preservation
- Triangle data integrity
- Checksum verification
- Load/save cycle accuracy

## Running Tests

### Method 1: Build Configuration (Compile-Time)

```bash
# In STM32CubeIDE:
1. Project → Properties
2. C/C++ Build → Settings → Preprocessor
3. Add: RUN_UNIT_TESTS
4. Build and flash
```

Output:
```
=== RUNNING UNIT TESTS ===
SD Card ready for testing

=== COLLISION MODULE TESTS ===
Running: test_box_intersection_overlap... PASS
Running: test_box_intersection_no_overlap... PASS
[...]
Tests run:    6
Tests passed: 6
Tests failed: 0

=== OBSTACLE MODULE TESTS ===
[...]

=== SD CARD MODULE TESTS ===
[...]

=== ALL TESTS COMPLETE ===
Total tests: 24
All PASSED!
```


## Writing New Tests

### Test Template

```c
// In test_yourmodule.c
#include "./Test/test_framework.h"
#include "./YourModule/yourmodule.h"

// Individual test function
uint8_t test_your_feature(void) {
    // Arrange
    YourStruct data;
    yourmodule_init(&data);
    
    // Act
    int result = yourmodule_function(&data, 42);
    
    // Assert
    TEST_ASSERT_EQUAL(expected_value, result, 
                      "Function should return expected_value");
    
    return 1;  // Test passed
}

// Test runner
void Run_YourModule_Tests(void) {
    UART_Printf("\r\n=== YOUR MODULE TESTS ===\r\n");
    
    test_stats.tests_run = 0;
    test_stats.tests_passed = 0;
    test_stats.tests_failed = 0;
    
    // Run tests
    RUN_TEST(test_your_feature);
    RUN_TEST(test_another_feature);
    
    // Report results
    UART_Printf("\r\nTests run: %lu\r\n", test_stats.tests_run);
    UART_Printf("Tests passed: %lu\r\n", test_stats.tests_passed);
    UART_Printf("Tests failed: %lu\r\n", test_stats.tests_failed);
}
```

### Best Practices

1. **Test Naming**: Use descriptive names `test_module_specific_behavior`
2. **Independence**: Each test should be independent
3. **Cleanup**: Restore state after each test
4. **Fast Execution**: Keep tests under 100ms each
5. **Clear Assertions**: Provide descriptive failure messages

## Coverage Analysis

### Uncovered Areas

- Error recovery paths in game logic
- ADC input edge cases
- SPI timeout handling
- Concurrent button press scenarios


### Regression Testing

After any code changes:
1. Run affected module tests
3. Verify game still plays normally
4. Check SD card operations

## Troubleshooting Tests

### Test Hanging

**Symptom**: Test suite stops mid-execution
**Common Causes**:
- SD card timeout (check connections)
- Infinite loop in test code
- Stack overflow

**Solution**:
```c
// Add timeout to long operations
uint32_t timeout = HAL_GetTick() + 1000;  // 1 second
while(condition && (HAL_GetTick() < timeout)) {
    // operation
}
```

### Inconsistent Results

**Symptom**: Tests pass/fail randomly
**Causes**:
- Uninitialized variables
- Timing dependencies
- Hardware issues

**Solution**:
```c
// Always initialize test data
memset(&test_struct, 0, sizeof(test_struct));
// Add delays for hardware operations
HAL_Delay(10);
```

### SD Card Tests Failing

**Checklist**:
1. Card inserted fully
2. 10kΩ pull-up on MISO
3. Both ground pins connected
4. Card formatted as FAT32
5. Card ≤32GB capacity

## Performance Benchmarks

### Memory Usage

- Test framework: 2KB code, 512B RAM
- Test data: 4KB temporary
- Stack usage: Max 1KB during tests


### Debug Helpers

```c
// Print test data in hex
void Debug_PrintHex(uint8_t* data, size_t len) {
    for(size_t i = 0; i < len; i++) {
        UART_Printf("%02X ", data[i]);
        if((i + 1) % 16 == 0) UART_Printf("\r\n");
    }
}
```

## References

- [Unity Test Framework](http://www.throwtheswitch.org/unity) - Inspiration
- [CppUTest](http://cpputest.github.io/) - Alternative framework

---

*Test Documentation Version 1.0*  
*Last Updated: November 2024*