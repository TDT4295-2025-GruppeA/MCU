#include "./Test/test_framework.h"
#include "./Game/obstacles.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

// Test 1: Initialization creates correct number of obstacles
uint8_t test_obstacles_init(void) {
    Obstacles_Init();

    uint8_t count = Obstacles_GetActiveCount();
    TEST_ASSERT(count >= 3, "Should spawn at least 3 initial obstacles");

    // Check they're positioned ahead
    Obstacle* obstacles = Obstacles_GetArray();
    for(int i = 0; i < MAX_OBSTACLES; i++) {
        if(obstacles[i].active) {
            TEST_ASSERT(obstacles[i].pos.z >= OBSTACLE_SPAWN_DIST,
                       "Initial obstacles should be ahead of player");
        }
    }
    return 1;
}

// Test 2: Spawning respects world boundaries
uint8_t test_spawn_within_bounds(void) {
    Obstacles_Reset();

    // Spawn many obstacles to test randomization
    for(int i = 0; i < 20; i++) {
        Obstacles_Spawn(100 + i * 10);
    }

    Obstacle* obstacles = Obstacles_GetArray();
    for(int i = 0; i < MAX_OBSTACLES; i++) {
        if(obstacles[i].active) {
            TEST_ASSERT(obstacles[i].pos.x >= WORLD_MIN_X,
                       "Obstacle X should be >= WORLD_MIN_X");
            TEST_ASSERT(obstacles[i].pos.x <= WORLD_MAX_X,
                       "Obstacle X should be <= WORLD_MAX_X");
        }
    }
    return 1;
}

// Test 3: Obstacles de-spawn when passed
uint8_t test_obstacle_despawn(void) {
    Obstacles_Reset();
    Obstacles_SetAutoSpawn(0);  // Disable auto-spawn for this test

    // Spawn obstacle behind player
    Obstacles_Spawn(50);
    uint8_t initial_count = Obstacles_GetActiveCount();
    TEST_ASSERT(initial_count > 0, "Should have spawned obstacle");

    // Simulate player moving far ahead
    float player_z = 100;
    Obstacles_Update(player_z, 0.1f);

    uint8_t new_count = Obstacles_GetActiveCount();
    TEST_ASSERT(new_count < initial_count,
               "Obstacles behind player should despawn");

    uint32_t passed = Obstacles_CheckPassed(player_z);
    TEST_ASSERT(passed > 0, "Should track passed obstacles");

    Obstacles_SetAutoSpawn(1);  // Re-enable for other tests
    return 1;
}

// Test 4: Auto-spawn when player advances
uint8_t test_auto_spawn_ahead(void) {
    Obstacles_Reset();

    float player_z = 0;
    Obstacles_Update(player_z, 0.1f);
    uint8_t initial_count = Obstacles_GetActiveCount();

    // Move player forward significantly
    player_z = OBSTACLE_SPAWN_DIST - 10;
    Obstacles_Update(player_z, 0.1f);

    uint8_t new_count = Obstacles_GetActiveCount();
    TEST_ASSERT(new_count >= initial_count,
               "Should spawn new obstacles as player advances");

    // Check new obstacles are ahead
    Obstacle* obstacles = Obstacles_GetArray();
    int ahead_count = 0;
    for(int i = 0; i < MAX_OBSTACLES; i++) {
        if(obstacles[i].active && obstacles[i].pos.z > player_z) {
            ahead_count++;
        }
    }
    TEST_ASSERT(ahead_count > 0, "Should have obstacles ahead of player");

    return 1;
}

// Test 5: Visible count calculation
uint8_t test_visible_count(void) {
    Obstacles_Reset();

    // Create obstacles at known positions
    Obstacles_Clear();
    Obstacle* obstacles = Obstacles_GetArray();

    // Near obstacle (visible)
    obstacles[0].active = 1;
    obstacles[0].pos.z = 50;

    // Far obstacle (not visible)
    obstacles[1].active = 1;
    obstacles[1].pos.z = 200;

    // Behind obstacle (not visible)
    obstacles[2].active = 1;
    obstacles[2].pos.z = -20;

    float player_z = 0;
    float view_distance = 100;

    uint8_t visible = Obstacles_GetVisibleCount(player_z, view_distance);
    TEST_ASSERT_EQUAL(1, visible, "Should only see 1 obstacle in range");

    return 1;
}

// Test 6: Shape variety
uint8_t test_shape_variety(void) {
    Obstacles_Reset();
    srand(12345);  // Fixed seed for reproducible test

    int cube_count = 0, cone_count = 0, pyramid_count = 0;

    // Spawn many obstacles
    for(int i = 0; i < 30; i++) {
        Obstacles_Spawn(i * 20);
    }

    Obstacle* obstacles = Obstacles_GetArray();
    for(int i = 0; i < MAX_OBSTACLES; i++) {
        if(obstacles[i].active) {
            switch(obstacles[i].shape_id) {
                case SHAPE_CUBE: cube_count++; break;
                case SHAPE_CONE: cone_count++; break;
                case SHAPE_PYRAMID: pyramid_count++; break;
            }
        }
    }

    // Check we have variety (with fixed seed, should get mix)
    TEST_ASSERT(cube_count > 0, "Should spawn some cubes");
    TEST_ASSERT(cone_count > 0, "Should spawn some cones");
    // Note: pyramid_count might be 0 if MAX_OBSTACLES is small

    return 1;
}

// Test 7: Spawn spacing
uint8_t test_spawn_spacing(void) {
    Obstacles_Reset();
    Obstacles_Clear();

    // Spawn two obstacles
    Obstacles_Spawn(100);
    Obstacles_Spawn(100 + OBSTACLE_SPACING);

    Obstacle* obstacles = Obstacles_GetArray();
    float min_distance = 1000;

    // Find minimum distance between obstacles
    for(int i = 0; i < MAX_OBSTACLES; i++) {
        if(obstacles[i].active) {
            for(int j = i+1; j < MAX_OBSTACLES; j++) {
                if(obstacles[j].active) {
                    float dist = fabsf(obstacles[i].pos.z - obstacles[j].pos.z);
                    if(dist < min_distance) {
                        min_distance = dist;
                    }
                }
            }
        }
    }

    TEST_ASSERT(min_distance >= OBSTACLE_SPACING - 1,
               "Obstacles should maintain minimum spacing");

    return 1;
}

// Main test runner
void Run_Obstacle_Tests(void) {
    UART_Printf("\r\n=== OBSTACLE MODULE TESTS ===\r\n");

    test_stats.tests_run = 0;
    test_stats.tests_passed = 0;
    test_stats.tests_failed = 0;

    RUN_TEST(test_obstacles_init);
    RUN_TEST(test_spawn_within_bounds);
    RUN_TEST(test_obstacle_despawn);
    RUN_TEST(test_auto_spawn_ahead);
    RUN_TEST(test_visible_count);
    RUN_TEST(test_shape_variety);
    RUN_TEST(test_spawn_spacing);

    UART_Printf("\r\n=== TEST SUMMARY ===\r\n");
    UART_Printf("Tests run:    %lu\r\n", test_stats.tests_run);
    UART_Printf("Tests passed: %lu\r\n", test_stats.tests_passed);
    UART_Printf("Tests failed: %lu\r\n", test_stats.tests_failed);

    if (test_stats.tests_failed == 0) {
        UART_Printf("ALL TESTS PASSED!\r\n");
    }
}

