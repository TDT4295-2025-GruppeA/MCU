#include "./Game/game.h"
#include "./Game/shapes.h"
#include "./Game/obstacles.h"
#include "./Game/collision.h"
#include "./Game/spi_protocol.h"
#include "./Game/input.h"
#include "./SDCard/sd_card.h"
#include "./SDCard/game_storage.h"
#include "buttons.h"
#include "main.h"
#include <string.h>

// External handles from main.c
extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi3;
extern UART_HandleTypeDef huart1;
extern void UART_Printf(const char* format, ...);

// Game state
static GameState game_state;
static uint32_t last_update_time = 0;
static uint32_t score_multiplier = 1;
static ADCButtonState adc_buttons;
static GameStats game_stats = {0};
static uint32_t game_start_time = 0;
static uint8_t sd_card_ready = 0;
static void Handle_Input(void);
static void Move_Player(float delta_x);
static void Update_GameLogic(float delta_time);
static void Check_Collisions(void);
static void Update_Score(void);
static void Render_Frame(void);



// Initialize game
void Game_Init(void)
{
    UART_Printf("\r\n=================================\r\n");
    UART_Printf("Flyboy 3D \r\n");
    UART_Printf("=================================\r\n");
    UART_Printf("Initializing subsystems...\r\n");

    // Initialize subsystems
    SPI_Protocol_Init(&hspi1);

    Input_Init();
    Buttons_Init();
    Shapes_Init();
    Obstacles_Init();

    UART_Printf("Initializing SD Card...\r\n");
	if(Storage_Init(&hspi3) == SD_OK) {
		sd_card_ready = 1;

		Storage_InitializeShapes();

		Game_LoadStats();
		UART_Printf("SD Card ready. High Score: %lu\r\n", game_stats.high_score);
	} else {
		sd_card_ready = 0;
		UART_Printf("SD Card not found - playing without saves\r\n");
	}


    // Send shapes to FPGA
    UART_Printf("Sending shapes to FPGA...\r\n");
    SPI_SendShapeToFPGA(Shapes_GetPlayer());
    SPI_SendShapeToFPGA(Shapes_GetCube());
    SPI_SendShapeToFPGA(Shapes_GetCone());

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
    SPI_ClearScene();

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
        game_start_time = HAL_GetTick();  // Track game start time

        SPI_SendGameState(game_state.state, game_state.score);
        UART_Printf("Game started! Beat high score: %lu\r\n", game_stats.high_score);
    }
}

// Pause the game
void Game_Pause(void)
{
    if(game_state.state == GAME_STATE_PLAYING)
    {
        game_state.state = GAME_STATE_PAUSED;
        game_state.moving_forward = 0;

        SPI_SendGameState(game_state.state, game_state.score);
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

        SPI_SendGameState(game_state.state, game_state.score);
        UART_Printf("Game resumed\r\n");
    }
}

// Game over
void Game_Over(void)
{
    game_state.state = GAME_STATE_GAME_OVER;
    game_state.moving_forward = 0;

    SPI_SendCollisionEvent();
    SPI_SendGameState(game_state.state, game_state.score);

    // Update statistics
    uint32_t game_time = (HAL_GetTick() - game_start_time) / 1000;  // seconds
    game_stats.total_games++;
    game_stats.total_time += game_time;
    game_stats.last_score = game_state.score;

    // Check for new high score
    uint8_t new_high_score = 0;
    if(game_state.score > game_stats.high_score) {
        game_stats.high_score = game_state.score;
        new_high_score = 1;
    }

    // Save to SD card
    if(sd_card_ready) {
        Game_SaveStats();
    }

    UART_Printf("\r\n*** GAME OVER ***\r\n");
    if(new_high_score) {
        UART_Printf("NEW HIGH SCORE: %lu!!\r\n", game_state.score);
    } else {
        UART_Printf("Score: %lu (High: %lu)\r\n", game_state.score, game_stats.high_score);
    }
    UART_Printf("Game Time: %lu seconds\r\n", game_time);
    UART_Printf("Obstacles Passed: %lu\r\n", Obstacles_CheckPassed(game_state.player_pos.z));
    UART_Printf("Total Games: %lu\r\n", game_stats.total_games);
    UART_Printf("Press button to play again\r\n\r\n");
}

void Game_SaveStats(void)
{
    if(!sd_card_ready) return;

    GameSave save;
    save.magic = 0x47414D45;  // "GAME"
    save.version = 1;
    save.high_score = game_stats.high_score;
    save.total_played = game_stats.total_games;
    save.total_time = game_stats.total_time;

    // Simple checksum
    save.checksum = save.high_score ^ save.total_played ^ save.total_time;

    if(Storage_SaveGame(&save) == SD_OK) {
        UART_Printf("Stats saved to SD card\r\n");
    } else {
        UART_Printf("Failed to save stats\r\n");
    }
}

void Game_LoadStats(void)
{
    if(!sd_card_ready) return;

    GameSave save;
    if(Storage_LoadGame(&save) == SD_OK) {
        // Verify checksum
        uint32_t check = save.high_score ^ save.total_played ^ save.total_time;
        if(check == save.checksum) {
            game_stats.high_score = save.high_score;
            game_stats.total_games = save.total_played;
            game_stats.total_time = save.total_time;
            UART_Printf("Stats loaded from SD card\r\n");
        } else {
            UART_Printf("Save file corrupted, starting fresh\r\n");
        }
    } else {
        UART_Printf("No save file found, creating new\r\n");
    }
}

void Game_Update(uint32_t current_time)
{
    // Check update interval
    if(current_time - last_update_time < UPDATE_INTERVAL) {
        return;
    }

    float delta_time = (current_time - last_update_time) / 1000.0f;
    last_update_time = current_time;
    game_state.frame_count++;

    // 1. INPUT HANDLING
    Handle_Input();

    // 2. GAME LOGIC (only if playing)
    if(game_state.state == GAME_STATE_PLAYING) {
        Update_GameLogic(delta_time);
        Check_Collisions();
        Update_Score();

        // 3. RENDERING TO FPGA
        Render_Frame();
    }
}

static void Handle_Input(void)
{
    Buttons_Update(&adc_buttons);

    switch(game_state.state) {
        case GAME_STATE_PLAYING:
            if(adc_buttons.left_pressed) {
                Move_Player(-STRAFE_SPEED);
            }
            if(adc_buttons.right_pressed) {
                Move_Player(STRAFE_SPEED);
            }
            if(adc_buttons.left_long_press) {
                Game_Start();
                UART_Printf("RESET (long press)\r\n");
            }
            break;

        case GAME_STATE_PAUSED:
            if(adc_buttons.both_pressed) {
                Game_Resume();
            }
            break;

        case GAME_STATE_MENU:
        case GAME_STATE_GAME_OVER:
            if(adc_buttons.left_pressed || adc_buttons.right_pressed) {
                Game_Start();
            }
            break;
    }
}

static void Move_Player(float delta_x)
{
    game_state.player_pos.x += delta_x;

    // Clamp to world bounds
    if(game_state.player_pos.x < WORLD_MIN_X) {
        game_state.player_pos.x = WORLD_MIN_X;
    } else if(game_state.player_pos.x > WORLD_MAX_X) {
        game_state.player_pos.x = WORLD_MAX_X;
    }

    UART_Printf("Player X=%d\r\n", (int)game_state.player_pos.x);
}

static void Update_GameLogic(float delta_time)
{
    // Move forward
    if(game_state.moving_forward) {
        game_state.player_pos.z += FORWARD_SPEED * delta_time;
    }

    // Update obstacles
    Obstacles_Update(game_state.player_pos.z, delta_time);
}

static void Check_Collisions(void)
{
    Obstacle* obstacles = Obstacles_GetArray();
    CollisionResult collision = Collision_CheckPlayer(&game_state.player_pos, obstacles, MAX_OBSTACLES);

    if(collision.type != COLLISION_NONE) {
        Game_Over();
    }
}

static void Update_Score(void)
{
    uint32_t obstacles_passed = Obstacles_CheckPassed(game_state.player_pos.z);
    game_state.score = (uint32_t)(game_state.player_pos.z) + (obstacles_passed * 10 * score_multiplier);
}

static void Render_Frame(void)
{
    // Clear previous frame instances
    // SPI_ClearScene();
    // 1. Send player as model instance
    SPI_AddModelInstance(SHAPE_ID_PLAYER, &game_state.player_pos, NULL);  // NULL = no rotation

    // 2. Send visible obstacles as model instances
    Obstacle* obstacles = Obstacles_GetArray();
    int instances_sent = 0;

    for(int i = 0; i < MAX_OBSTACLES && instances_sent < 15; i++) {  // FPGA limit
        if(obstacles[i].active) {
            // Calculate relative position (camera-space)
            Position relative_pos = obstacles[i].pos;
            relative_pos.z -= game_state.player_pos.z;

            // Only send if visible
            if(relative_pos.z > -20 && relative_pos.z < 150) {
                SPI_AddModelInstance(obstacles[i].shape_id, &relative_pos, NULL);
                instances_sent++;
            }
        }
    }

    // 3. Tell FPGA to render the frame
    SPI_BeginRender();
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
