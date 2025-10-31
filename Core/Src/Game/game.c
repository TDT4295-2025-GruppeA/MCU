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

// External handles
extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi3;
extern void UART_Printf(const char* format, ...);

// Forward declarations of static functions
static void _HandleInput(void);

// Global game state
static GameState game_state;
static uint32_t last_update_time = 0;
static ADCButtonState adc_buttons;

void Game_Init(void)
{
    UART_Printf("\r\n=================================\r\n");
    UART_Printf("Cubefield  \r\n");
    UART_Printf("=================================\r\n");

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
        HAL_Delay(1000);
    }
}

// Static function implementation
static void _HandleInput(void)
{
    switch(game_state.state) {
        case GAME_STATE_PLAYING:
            if(adc_buttons.left_pressed) {
                GameLogic_MovePlayer(&game_state, -STRAFE_SPEED);
            }
            if(adc_buttons.right_pressed) {
                GameLogic_MovePlayer(&game_state, STRAFE_SPEED);
            }
            if(adc_buttons.left_long_press) {
                StateManager_TransitionTo(GAME_STATE_PLAYING);
            }
            break;

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

void Game_HandleButton(uint8_t button_state, uint32_t current_time)
{
    InputAction action = Input_ProcessButton(button_state, current_time);

    switch(action) {
        case ACTION_MOVE_LEFT:
            if(game_state.state == GAME_STATE_PLAYING) {
                GameLogic_MovePlayer(&game_state, -STRAFE_SPEED);
            }
            break;

        case ACTION_MOVE_RIGHT:
            if(game_state.state == GAME_STATE_PLAYING) {
                GameLogic_MovePlayer(&game_state, STRAFE_SPEED);
            }
            break;

        case ACTION_PAUSE:
            if(game_state.state == GAME_STATE_PLAYING) {
                StateManager_Pause();
            } else if(game_state.state == GAME_STATE_PAUSED) {
                StateManager_Resume();
            }
            break;

        default:
            break;
    }
}
