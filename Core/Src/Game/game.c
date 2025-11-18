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

// Input mode: 0=binary, 1=analog
static uint8_t input_mode = 1;

// External handles
extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi3;
extern void UART_Printf(const char* format, ...);

// Forward declarations of static functions
static void _HandleInput(float delta_time);

// Helper: update player strafe movement (acceleration-based, supports joystick)
static void UpdatePlayerStrafe(GameState* state, float delta_time, float input)
{
    // input: -1 (left), 0 (none), 1 (right), or analog [-1,1] for joystick
    float target_speed = PLAYER_STRAFE_MAX_SPEED * input;

    float error = state->player_strafe_speed - target_speed;

    // Accelerate based on input
    state->player_strafe_speed -= PLAYER_STRAFE_ACCEL * error * delta_time;

    // Move player by current speed
    GameLogic_MovePlayer(state, state->player_strafe_speed * delta_time);
}

// Global game state
GameState game_state;
static uint32_t last_update_time = 0;
static uint32_t last_render_time = 0;
static ADCButtonState adc_buttons;

void Game_SetInputMode(uint8_t mode) {
    input_mode = mode; // 0=binary, 1=analog
}

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
    _HandleInput(delta_time);

    // Update game if playing
    if(game_state.state == GAME_STATE_PLAYING) {
        GameLogic_Update(&game_state, delta_time);

        if(GameLogic_CheckCollisions(&game_state)) {
        	StateManager_GameOver();
        	UART_Printf("Collision event!  \r\n");
        } else {
            GameLogic_UpdateScore(&game_state);
        }
    }

    // Render slower than game loop.
    if (current_time - last_render_time > RENDER_INTERVAL) {
        last_render_time = current_time;
        Renderer_DrawFrame(&game_state);
    }

}

// Static function implementation
static void _HandleInput(float delta_time)
{
    switch(game_state.state) {
        case GAME_STATE_PLAYING: {
            float player_input = 0.0f;
            if(input_mode == 0) {
                // Binary mode (left/right)
                if(adc_buttons.left) {
                    player_input = -1.0f;
                } else if(adc_buttons.right) {
                    player_input = 1.0f;
                } else {
                    player_input = 0.0f;
                }
            } else {
                // Analog mode (full range)
                uint32_t adc_val = Buttons_GetLastADCValue();

                if (adc_val < POT_CENTER - POT_SPAN) {
                    adc_val = POT_CENTER - POT_SPAN;
                } else if (adc_val > POT_CENTER + POT_SPAN) {
                    adc_val = POT_CENTER + POT_SPAN;
                }

                float norm = -((float)adc_val - (float)POT_CENTER) / (POT_SPAN);
                float edge_deadzone = (float)POT_DEADZONE / (float)POT_CENTER;
                if(fabsf((float)adc_val - (float)POT_CENTER) < (float)POT_DEADZONE) {
                    player_input = 0.0f;
                } else if(norm > 1.0f - edge_deadzone) {
                    player_input = 1.0f;
                } else if(norm < -1.0f + edge_deadzone) {
                    player_input = -1.0f;
                } else {
                    player_input = norm / 2.0f;
                }
            }
            UpdatePlayerStrafe(&game_state, delta_time, player_input);
            if(adc_buttons.left_long_press || adc_buttons.right_long_press) {
                StateManager_TransitionTo(GAME_STATE_PLAYING);
            }
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

