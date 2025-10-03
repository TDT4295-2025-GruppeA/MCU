#include "./Test/test_framework.h"
#include "./Game/collision.h"
#include <math.h>
#include <string.h>

TestStats test_stats = {0, 0, 0};

// Test 1: Box intersection when boxes overlap
uint8_t test_box_intersection_overlap(void) {
    Position pos1 = {0, 0, 0};
    Position pos2 = {5, 0, 0};  // Overlapping in X

    uint8_t result = Collision_BoxIntersect(&pos1, 10, 10, 10,
                                           &pos2, 10, 10, 10);

    TEST_ASSERT_EQUAL(1, result, "Overlapping boxes should collide");
    return 1;
}

// Test 2: Box intersection when boxes don't overlap
uint8_t test_box_intersection_no_overlap(void) {
    Position pos1 = {0, 0, 0};
    Position pos2 = {20, 0, 0};  // Too far apart

    uint8_t result = Collision_BoxIntersect(&pos1, 10, 10, 10,
                                           &pos2, 10, 10, 10);

    TEST_ASSERT_EQUAL(0, result, "Non-overlapping boxes shouldn't collide");
    return 1;
}

// Test 3: Boundary collision detection
uint8_t test_boundary_collision(void) {
    Position player_pos = {WORLD_MIN_X - 1, 0, 0};  // Outside left boundary
    Obstacle obstacles[1] = {0};

    CollisionResult result = Collision_CheckPlayer(&player_pos, obstacles, 0);

    TEST_ASSERT_EQUAL(COLLISION_BOUNDARY, result.type,
                      "Should detect left boundary collision");

    // Test right boundary
    player_pos.x = WORLD_MAX_X + 1;
    result = Collision_CheckPlayer(&player_pos, obstacles, 0);

    TEST_ASSERT_EQUAL(COLLISION_BOUNDARY, result.type,
                      "Should detect right boundary collision");
    return 1;
}

// Test 4: Player-obstacle collision
uint8_t test_player_obstacle_collision(void) {
    Position player_pos = {0, 0, 0};

    Obstacle obstacles[2];
    memset(obstacles, 0, sizeof(obstacles));

    // First obstacle - colliding
    obstacles[0].active = 1;
    obstacles[0].pos.x = 0;
    obstacles[0].pos.y = 0;
    obstacles[0].pos.z = 0;
    obstacles[0].width = 10;
    obstacles[0].height = 10;
    obstacles[0].depth = 10;

    // Second obstacle - not colliding
    obstacles[1].active = 1;
    obstacles[1].pos.x = 100;
    obstacles[1].pos.y = 0;
    obstacles[1].pos.z = 0;
    obstacles[1].width = 10;
    obstacles[1].height = 10;
    obstacles[1].depth = 10;

    CollisionResult result = Collision_CheckPlayer(&player_pos, obstacles, 2);

    TEST_ASSERT_EQUAL(COLLISION_OBSTACLE, result.type,
                      "Should detect obstacle collision");
    TEST_ASSERT_EQUAL(0, result.obstacle_index,
                      "Should collide with first obstacle");
    return 1;
}

// Test 5: No collision scenario
uint8_t test_no_collision(void) {
    Position player_pos = {0, 0, 0};

    Obstacle obstacles[1];
    obstacles[0].active = 1;
    obstacles[0].pos.x = 100;  // Far away
    obstacles[0].pos.y = 100;
    obstacles[0].pos.z = 100;
    obstacles[0].width = 10;
    obstacles[0].height = 10;
    obstacles[0].depth = 10;

    CollisionResult result = Collision_CheckPlayer(&player_pos, obstacles, 1);

    TEST_ASSERT_EQUAL(COLLISION_NONE, result.type,
                      "Should detect no collision");
    return 1;
}

// Test 6: Edge case - exact boundary
uint8_t test_exact_boundary(void) {
    Position pos1 = {0, 0, 0};
    Position pos2 = {10, 0, 0};  // Exactly touching (boxes are 10 wide, centers 10 apart)

    uint8_t result = Collision_BoxIntersect(&pos1, 10, 10, 10,
                                           &pos2, 10, 10, 10);

    TEST_ASSERT_EQUAL(0, result, "Exactly touching boxes shouldn't collide");
    return 1;
}

// Main test runner
void Run_Collision_Tests(void) {
    UART_Printf("\r\n=== COLLISION MODULE TESTS ===\r\n");

    // Reset stats
    test_stats.tests_run = 0;
    test_stats.tests_passed = 0;
    test_stats.tests_failed = 0;

    // Run all tests
    RUN_TEST(test_box_intersection_overlap);
    RUN_TEST(test_box_intersection_no_overlap);
    RUN_TEST(test_boundary_collision);
    RUN_TEST(test_player_obstacle_collision);
    RUN_TEST(test_no_collision);
    RUN_TEST(test_exact_boundary);

    // Print summary
    UART_Printf("\r\n=== TEST SUMMARY ===\r\n");
    UART_Printf("Tests run:    %lu\r\n", test_stats.tests_run);
    UART_Printf("Tests passed: %lu\r\n", test_stats.tests_passed);
    UART_Printf("Tests failed: %lu\r\n", test_stats.tests_failed);

    if (test_stats.tests_failed == 0) {
        UART_Printf("ALL TESTS PASSED!\r\n");
    } else {
        UART_Printf("SOME TESTS FAILED!\r\n");
    }
}
