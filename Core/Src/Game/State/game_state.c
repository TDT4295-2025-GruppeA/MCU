#include "../../../Inc/Game/State/state_manager.h"
#include "../../../Inc/Game/Rendering/rendering.h"
#include "../../../Inc/Game/Persistence/save_system.h"
#include "../../../Inc/Game/obstacles.h"
#include "../../../Inc/Game/spi_protocol.h"

#include "main.h"
#include <string.h>

extern void UART_Printf(const char* format, ...);
static void _HandleGameOver(void);

// Static variables
static GameState* game_state_ptr = NULL;
static uint32_t score_multiplier = 1;

void StateManager_Init(GameState* state)
{
    game_state_ptr = state;
    StateManager_Reset();
}

void StateManager_Reset(void)
{
    if(!game_state_ptr) return;

    // Reset game state
    memset(game_state_ptr, 0, sizeof(GameState));
    game_state_ptr->player_pos.x = 0;
    game_state_ptr->player_pos.y = 0;
    game_state_ptr->player_pos.z = 0;
    game_state_ptr->state = GAME_STATE_MENU;
    game_state_ptr->score = 0;
    game_state_ptr->moving_forward = 0;
    game_state_ptr->frame_count = 0;
    game_state_ptr->total_distance = 0.0;
    score_multiplier = 1;

    // Reset subsystems
    Obstacles_Reset();
    Renderer_ClearScene();

    BSP_LED_Off(LED_GREEN);
    UART_Printf("Game reset\r\n");
}

void StateManager_TransitionTo(GameStateEnum new_state)
{
    if(!game_state_ptr) return;

    GameStateEnum old_state = game_state_ptr->state;

    // Handle state exit logic
    switch(old_state) {
        case GAME_STATE_PLAYING:
            game_state_ptr->moving_forward = 0;
            break;
        default:
            break;
    }

    // Transition to new state
    game_state_ptr->state = new_state;

    // Handle state entry logic
    switch(new_state) {
        case GAME_STATE_PLAYING:
            if(old_state == GAME_STATE_MENU || old_state == GAME_STATE_GAME_OVER) {
                StateManager_Reset();
                game_state_ptr->state = GAME_STATE_PLAYING;
            }
            game_state_ptr->moving_forward = 1;
            game_state_ptr->game_start_time = HAL_GetTick();
            UART_Printf("Game started! Beat high score: %lu\r\n",
                       SaveSystem_GetHighScore());
            break;

        case GAME_STATE_PAUSED:
            game_state_ptr->moving_forward = 0;
            UART_Printf("Game paused\r\n");
            break;

        case GAME_STATE_GAME_OVER:
            _HandleGameOver();
            break;

        default:
            break;
    }

    // Notify FPGA of state change
    SPI_SendGameState(game_state_ptr->state, game_state_ptr->score);
}

void StateManager_Pause(void)
{
    if(game_state_ptr && game_state_ptr->state == GAME_STATE_PLAYING) {
        StateManager_TransitionTo(GAME_STATE_PAUSED);
    }
}

void StateManager_Resume(void)
{
    if(game_state_ptr && game_state_ptr->state == GAME_STATE_PAUSED) {
        StateManager_TransitionTo(GAME_STATE_PLAYING);
    }
}

void StateManager_GameOver(void)
{
    StateManager_TransitionTo(GAME_STATE_GAME_OVER);
}

static void _HandleGameOver(void)
{
    if(!game_state_ptr) return;

    game_state_ptr->moving_forward = 0;

    // Send collision event to FPGA
    SPI_SendCollisionEvent();

    // Calculate game time
    uint32_t game_time = (HAL_GetTick() - game_state_ptr->game_start_time) / 1000;

    // Update and save statistics
    SaveSystem_RecordGame(game_state_ptr->score, game_time);

    // Display game over message
    uint32_t obstacles_passed = Obstacles_CheckPassed(game_state_ptr->player_pos.z);

    UART_Printf("\r\n*** GAME OVER ***\r\n");
    if(SaveSystem_IsNewHighScore(game_state_ptr->score)) {
        UART_Printf("NEW HIGH SCORE: %lu!!\r\n", game_state_ptr->score);
    } else {
        UART_Printf("Score: %lu (High: %lu)\r\n",
                   game_state_ptr->score, SaveSystem_GetHighScore());
    }
    UART_Printf("Game Time: %lu seconds\r\n", game_time);
    UART_Printf("Obstacles Passed: %lu\r\n", obstacles_passed);
    UART_Printf("Press button to play again\r\n\r\n");
}

GameStateEnum StateManager_GetCurrent(void)
{
    return game_state_ptr ? game_state_ptr->state : GAME_STATE_MENU;
}

const char* StateManager_GetStateName(GameStateEnum state)
{
    switch(state) {
        case GAME_STATE_MENU:     return "MENU";
        case GAME_STATE_PLAYING:  return "PLAYING";
        case GAME_STATE_PAUSED:   return "PAUSED";
        case GAME_STATE_GAME_OVER: return "GAME_OVER";
        default:                  return "UNKNOWN";
    }
}
