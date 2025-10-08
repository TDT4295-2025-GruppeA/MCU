#include "./Game/game.h"
#include "./Game/shapes.h"
#include "./Game/obstacles.h"
#include "./Game/collision.h"
#include "./Game/spi_protocol.h"
#include "./Game/input.h"
#include "buttons.h"
#include "main.h"
#include <string.h>

// External handles from main.c
extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;
extern void UART_Printf(const char* format, ...);

// Game state
static GameState game_state;
static uint32_t last_update_time = 0;
static uint32_t score_multiplier = 1;
static ADCButtonState adc_buttons;


// Initialize game
void Game_Init(void)
{
    UART_Printf("\r\n=================================\r\n");
    UART_Printf("3D TUNNEL RUNNER - MODULAR EDITION\r\n");
    UART_Printf("=================================\r\n");
    UART_Printf("Initializing subsystems...\r\n");

    // Initialize subsystems
    SPI_Protocol_Init(&hspi1);
    Input_Init();
    Buttons_Init();
    Shapes_Init();
    Obstacles_Init();

    // Send shapes to FPGA
    UART_Printf("Sending shapes to FPGA...\r\n");
    SPI_SendShape(Shapes_GetPlayer());
    SPI_SendShape(Shapes_GetCube());
    SPI_SendShape(Shapes_GetCone());

    // Initialize game state
    Game_Reset();

    UART_Printf("\r\nControls:\r\n");
    UART_Printf("  Button: 1 click = LEFT\r\n");
    UART_Printf("         2 clicks = RIGHT\r\n");
    UART_Printf("         Hold = PAUSE/RESUME\r\n");
    UART_Printf("  Keys: A/D = left/right\r\n");
    UART_Printf("        S = pause, W = resume\r\n");
    UART_Printf("        R = reset\r\n");
    UART_Printf("\r\nGame ready! Starting...\r\n\r\n");

    Game_Start();
}

// Reset game to initial state
void Game_Reset(void)
{
    memset(&game_state, 0, sizeof(GameState));

    game_state.player_pos.x = 0;
    game_state.player_pos.y = 0;
    game_state.player_pos.z = 0;
    game_state.state = GAME_STATE_MENU;
    game_state.score = 0;
    game_state.moving_forward = 0;
    game_state.frame_count = 0;

    last_update_time = HAL_GetTick();
    score_multiplier = 1;

    // Reset subsystems
    Obstacles_Reset();
    // SPI_ClearScene();

    BSP_LED_Off(LED_GREEN);


    UART_Printf("Game reset\r\n");
}

// Start the game
void Game_Start(void)
{
    if(game_state.state == GAME_STATE_MENU || game_state.state == GAME_STATE_GAME_OVER)
    {
        Game_Reset();
        game_state.state = GAME_STATE_PLAYING;
        game_state.moving_forward = 1;

    // SPI_SendGameState(game_state.state, game_state.score);
        UART_Printf("Game started!\r\n");
    }
}

// Pause the game
void Game_Pause(void)
{
    if(game_state.state == GAME_STATE_PLAYING)
    {
        game_state.state = GAME_STATE_PAUSED;
        game_state.moving_forward = 0;

    // SPI_SendGameState(game_state.state, game_state.score);
        UART_Printf("Game paused\r\n");
    }
}

// Resume the game
void Game_Resume(void)
{
    if(game_state.state == GAME_STATE_PAUSED)
    {
        game_state.state = GAME_STATE_PLAYING;
        game_state.moving_forward = 1;

    // SPI_SendGameState(game_state.state, game_state.score);
        UART_Printf("Game resumed\r\n");
    }
}

// Game over
void Game_Over(void)
{
    game_state.state = GAME_STATE_GAME_OVER;
    game_state.moving_forward = 0;

    SPI_SendCollisionEvent();
    // SPI_SendGameState(game_state.state, game_state.score);


    UART_Printf("\r\n*** GAME OVER ***\r\n");
    UART_Printf("Final Score: %lu\r\n", game_state.score);
    UART_Printf("Obstacles Passed: %lu\r\n", Obstacles_CheckPassed(game_state.player_pos.z));
    UART_Printf("Press R to restart\r\n\r\n");
}

// Main update loop
void Game_Update(uint32_t current_time)
{
    // Check update interval
    if(current_time - last_update_time < UPDATE_INTERVAL)
    {
        return;
    }

    // UPDATE ADC BUTTONS
    Buttons_Update(&adc_buttons);

    // Handle button input based on game state
    if(game_state.state == GAME_STATE_PLAYING)
    {
        // Left button pressed
        if(adc_buttons.left_pressed)
        {
            game_state.player_pos.x -= STRAFE_SPEED;
            if(game_state.player_pos.x < WORLD_MIN_X) {
                game_state.player_pos.x = WORLD_MIN_X;
            }
            UART_Printf("LEFT -> X=%d\r\n", (int)game_state.player_pos.x);
        }

        // Right button pressed
        if(adc_buttons.right_pressed)
        {
            game_state.player_pos.x += STRAFE_SPEED;
            if(game_state.player_pos.x > WORLD_MAX_X) {
                game_state.player_pos.x = WORLD_MAX_X;
            }
            UART_Printf("RIGHT -> X=%d\r\n", (int)game_state.player_pos.x);
        }

        // Long press left = reset
        if(adc_buttons.left_long_press)
        {
            Game_Start();
            UART_Printf("RESET (long press)\r\n");
        }
    }
    else if(game_state.state == GAME_STATE_PAUSED)
    {
        // Both buttons to resume
        if(adc_buttons.both_pressed)
        {
            Game_Resume();
        }
    }
    else if(game_state.state == GAME_STATE_GAME_OVER || game_state.state == GAME_STATE_MENU)
    {
        // Any button to start
        if(adc_buttons.left_pressed || adc_buttons.right_pressed)
        {
            Game_Start();
        }
    }

    float delta_time = (current_time - last_update_time) / 1000.0f;
    last_update_time = current_time;
    game_state.frame_count++;

    // Only update game logic if playing
    if(game_state.state != GAME_STATE_PLAYING)
    {
        return;
    }

    // Move forward if enabled
    if(game_state.moving_forward)
    {
        game_state.player_pos.z += FORWARD_SPEED * delta_time;
    }

    // Keep player in bounds
    if(game_state.player_pos.x < WORLD_MIN_X) game_state.player_pos.x = WORLD_MIN_X;
    if(game_state.player_pos.x > WORLD_MAX_X) game_state.player_pos.x = WORLD_MAX_X;

    // Update obstacles
    Obstacles_Update(game_state.player_pos.z, delta_time);

    // Check collisions
    Obstacle* obstacles = Obstacles_GetArray();
    CollisionResult collision = Collision_CheckPlayer(&game_state.player_pos, obstacles, MAX_OBSTACLES);

    // Update score
    uint32_t obstacles_passed = Obstacles_CheckPassed(game_state.player_pos.z);
    game_state.score = (uint32_t)(game_state.player_pos.z) + (obstacles_passed * 10 * score_multiplier);

    // Print status periodically
    if(game_state.frame_count % 10 == 0)
    {
        UART_Printf("[%lu] Pos: X=%d Z=%d | Score: %lu | Obstacles: %d active\r\n",
                   game_state.frame_count,
                   (int)game_state.player_pos.x,
                   (int)game_state.player_pos.z,
                   game_state.score,
                   Obstacles_GetActiveCount());
    }

    // Send updates to FPGA
    SPI_SendPosition(&game_state.player_pos);

    // Send visible obstacles
    uint8_t visible_count = 0;
    Obstacle relative_obstacles[MAX_OBSTACLES];

    for(int i = 0; i < MAX_OBSTACLES; i++)
    {
        if(obstacles[i].active)
        {
            float rel_z = obstacles[i].pos.z - game_state.player_pos.z;
            if(rel_z > -20 && rel_z < 150)
            {
                relative_obstacles[visible_count] = obstacles[i];
                relative_obstacles[visible_count].pos.z = rel_z;
                visible_count++;
            }
        }
    }

    if(visible_count > 0)
    {
        SPI_SendObstacles(relative_obstacles, visible_count);
    }
}
// Handle button input
void Game_HandleButton(uint8_t button_state, uint32_t current_time)
{
    InputAction action = Input_ProcessButton(button_state, current_time);

    switch(action)
    {
        case ACTION_MOVE_LEFT:
            if(game_state.state == GAME_STATE_PLAYING)
            {
                game_state.player_pos.x -= STRAFE_SPEED;
                UART_Printf("Move LEFT -> X=%d\r\n", (int)game_state.player_pos.x);
            }
            break;

        case ACTION_MOVE_RIGHT:
            if(game_state.state == GAME_STATE_PLAYING)
            {
                game_state.player_pos.x += STRAFE_SPEED;
                UART_Printf("Move RIGHT -> X=%d\r\n", (int)game_state.player_pos.x);
            }
            break;

        case ACTION_PAUSE:
            if(game_state.state == GAME_STATE_PLAYING)
            {
                Game_Pause();
            }
            else if(game_state.state == GAME_STATE_PAUSED)
            {
                Game_Resume();
            }
            break;

        default:
            break;
    }
}

// Handle keyboard input
void Game_HandleKeyboard(uint8_t key)
{
    InputAction action = Input_ProcessKeyboard(key);

    switch(action)
    {
        case ACTION_MOVE_LEFT:
            if(game_state.state == GAME_STATE_PLAYING)
            {
                game_state.player_pos.x -= STRAFE_SPEED;
            }
            break;

        case ACTION_MOVE_RIGHT:
            if(game_state.state == GAME_STATE_PLAYING)
            {
                game_state.player_pos.x += STRAFE_SPEED;
            }
            break;

        case ACTION_PAUSE:
            Game_Pause();
            break;

        case ACTION_RESUME:
            Game_Resume();
            break;

        case ACTION_RESET:
            Game_Start();
            break;

        default:
            break;
    }
}

// Get game state
GameState* Game_GetState(void)
{
    return &game_state;
}

// Get current score
uint32_t Game_GetScore(void)
{
    return game_state.score;
}

// Check if game is running
uint8_t Game_IsRunning(void)
{
    return (game_state.state == GAME_STATE_PLAYING);
}
