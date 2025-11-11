//game.c
#include "../../Inc/Game/game.h"
#include "../../Inc/Game/State/state_manager.h"
#include "../../Inc/Game/Rendering/rendering.h"
#include "../../Inc/Game/Persistence/save_system.h"
#include "../../Inc/Game/Logic/game_logic.h"
#include "../../Inc/Game/input.h"
#include "../../Inc/Game/shapes.h"
#include "../../Inc/Game/obstacles.h"
#include "../../Inc/buttons.h"
#include "main.h"
#include <string.h>
#include <stdint.h>

#define TIME_STEP ((float)UPDATE_INTERVAL / 1000.0f) // seconds per frame

// External handles
extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi3;
extern void UART_Printf(const char* format, ...);

// Forward declarations of static functions
static void _HandleInput(void);

// Helper: update player strafe movement (acceleration-based, supports joystick)
static void UpdatePlayerStrafe(GameState* state, float input)
{
    // input: -1 (left), 0 (none), 1 (right), or analog [-1,1] for joystick
    float accel = PLAYER_STRAFE_ACCEL;
    float decel = PLAYER_STRAFE_DECEL;
    float max_speed = PLAYER_STRAFE_MAX_SPEED;

    // Accelerate based on input
    state->player_strafe_speed += accel * input * TIME_STEP;
    // Clamp speed
    if (state->player_strafe_speed > max_speed)
        state->player_strafe_speed = max_speed;
    if (state->player_strafe_speed < -max_speed)
        state->player_strafe_speed = -max_speed;

    // Decay speed toward zero if no input (simulate friction)
    if (input == 0) {
        if (state->player_strafe_speed > 0) {
            state->player_strafe_speed -= decel * TIME_STEP;
            if (state->player_strafe_speed < 0)
                state->player_strafe_speed = 0;
        } else if (state->player_strafe_speed < 0) {
            state->player_strafe_speed += decel * TIME_STEP;
            if (state->player_strafe_speed > 0)
                state->player_strafe_speed = 0;
        }
    }

    // Move player by current speed
    GameLogic_MovePlayer(state, state->player_strafe_speed * TIME_STEP);
}

// Global game state
GameState game_state;
static uint32_t last_update_time = 0;
static ADCButtonState adc_buttons;

void Game_Init(void)
{
    UART_Printf("\r\n=================================\r\n");
    UART_Printf("Cubefield  \r\n");
    UART_Printf("=================================\r\n");

    // srand(HAL_GetTick());

    // Initialize core systems
    Input_Init();
    Buttons_Init();
    Shapes_Init();
    Obstacles_Init();

    // Initialize feature modules
    StateManager_Init(&game_state);
    Renderer_Init(&hspi1);
    // SaveSystem_Init(&hspi3);
    GameLogic_Init();

    // Load saved data
    // SaveSystem_LoadStats();

    // Upload shapes to FPGA
    Renderer_UploadShapes();

    UART_Printf("Game ready! Starting...\r\n\r\n");
    StateManager_TransitionTo(GAME_STATE_PLAYING);

    game_state.player_strafe_speed = 0.0f;
}

void Game_Update(uint32_t current_time)
{
    if(current_time - last_update_time < UPDATE_INTERVAL) {
        return;
    }

    float delta_time = (current_time - last_update_time) / 1000.0f;
    last_update_time = current_time;
    game_state.frame_count++;

    // Handle input
    Buttons_Update(&adc_buttons);
    _HandleInput();

    // Update game if playing
    if(game_state.state == GAME_STATE_PLAYING) {
        GameLogic_Update(&game_state, delta_time);

        if(GameLogic_CheckCollisions(&game_state)) {
        	StateManager_GameOver();
        	UART_Printf("Collision event!  \r\n");
        } else {
            GameLogic_UpdateScore(&game_state);
            Renderer_DrawFrame(&game_state);

        }
    }
}

// Static function implementation
static void _HandleInput(void)
{
    switch(game_state.state) {
        case GAME_STATE_PLAYING: {
            // Button input: right=1, left=-1, none=0
            float input = 0.0f;
            if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == 0) {
                // input = 1.0f; // right
            } else {
                input = -1.0f; // left
            }
            // For joystick: input = joystick_value [-1,1]
            UpdatePlayerStrafe(&game_state, input);
            break;
        }

        case GAME_STATE_PAUSED:
            if(adc_buttons.both_pressed) {
                StateManager_Resume();
            }
            break;

        case GAME_STATE_MENU:
        case GAME_STATE_GAME_OVER:
            if(adc_buttons.left_pressed || adc_buttons.right_pressed) {
                StateManager_TransitionTo(GAME_STATE_PLAYING);
            }
            break;
    }
}

// Delegation functions
void Game_Reset(void) { StateManager_Reset(); }
void Game_Pause(void) { StateManager_Pause(); }
void Game_Resume(void) { StateManager_Resume(); }
void Game_Start(void) { StateManager_TransitionTo(GAME_STATE_PLAYING); }
void Game_Over(void) { StateManager_GameOver(); }

// Query functions
GameState* Game_GetState(void) { return &game_state; }
uint32_t Game_GetScore(void) { return game_state.score; }
uint8_t Game_IsRunning(void) { return (game_state.state == GAME_STATE_PLAYING); }

