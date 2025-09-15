/*
 * game_types.h
 *
 *  Created on: Sep 15, 2025
 *      Author: jornik
 */

// game_types.h - Common game types and constants
#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include <stdint.h>

// ========== Game Constants ==========
#define UPDATE_INTERVAL     100     // ms between updates
#define FORWARD_SPEED       5.0f    // units per second
#define STRAFE_SPEED        10      // units per strafe
#define WORLD_MIN_X         -50
#define WORLD_MAX_X         50

// 3D Shape constants
#define MAX_VERTICES        32      // Max vertices per shape
#define MAX_TRIANGLES       16      // Max triangles per shape
#define MAX_OBSTACLES       10      // Max obstacles on screen
#define OBSTACLE_SPAWN_DIST 100     // Distance ahead to spawn obstacles
#define OBSTACLE_SPACING    30      // Min spacing between obstacles

// ========== SPI Protocol Commands ==========
typedef enum {
    CMD_POSITION     = 0x01,   // Update player position
    CMD_SHAPE_DEF    = 0x02,   // Define a shape
    CMD_OBSTACLE_POS = 0x03,   // Update obstacle positions
    CMD_CLEAR_SCENE  = 0x04,   // Clear all objects
    CMD_COLLISION    = 0x05,   // Collision event
    CMD_GAME_STATE   = 0x06    // Game state update
} SPI_Command;

// ========== Shape IDs ==========
typedef enum {
    SHAPE_PLAYER = 0x00,
    SHAPE_CUBE   = 0x01,
    SHAPE_CONE   = 0x02,
    SHAPE_PYRAMID = 0x03,
    SHAPE_SPHERE = 0x04
} ShapeID;

// ========== Basic Types ==========
typedef struct {
    float x;
    float y;
    float z;
} Position;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} Vertex3D;

typedef struct {
    uint8_t v1;
    uint8_t v2;
    uint8_t v3;
} Triangle;

// ========== Shape Definition ==========
typedef struct {
    uint8_t id;                         // Shape identifier
    uint8_t vertex_count;               // Number of vertices
    uint8_t triangle_count;             // Number of triangles
    Vertex3D vertices[MAX_VERTICES];   // Vertex array
    Triangle triangles[MAX_TRIANGLES]; // Triangle array
} Shape3D;

// ========== Game Objects ==========
typedef struct {
    uint8_t active;         // Is this obstacle active?
    uint8_t shape_id;       // Which shape to use
    Position pos;           // Position in world
    float width;            // Collision box width
    float height;           // Collision box height
    float depth;            // Collision box depth
} Obstacle;

typedef struct {
    uint8_t pressed;
    uint32_t press_time;
    uint32_t release_time;
    uint8_t press_count;
} ButtonState;

// ========== Game State ==========
typedef enum {
    GAME_STATE_MENU,
    GAME_STATE_PLAYING,
    GAME_STATE_PAUSED,
    GAME_STATE_GAME_OVER
} GameStateEnum;

typedef struct {
    Position player_pos;
    uint8_t moving_forward;
    uint32_t frame_count;
    GameStateEnum state;
    uint32_t score;
    float next_spawn_z;
} GameState;

#endif // GAME_TYPES_H
