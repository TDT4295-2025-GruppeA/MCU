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
    // Send shapes to FPGA
    UART_Printf("Sending shapes to FPGA...\r\n");
    // SPI_SendShapeToFPGA(Shapes_GetPlayer());
    // SPI_SendShapeToFPGA(Shapes_GetCube());
    // SPI_SendShapeToFPGA(Shapes_GetCone());

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

    // TODO: SPI_SendCollisionEvent() used the old protocol. Commenting out
    // for now until collision event support is implemented in the new protocol.
    // SPI_SendCollisionEvent();
    // SPI_SendGameState(game_state.state, game_state.score);


    UART_Printf("\r\n*** GAME OVER ***\r\n");
    UART_Printf("Final Score: %lu\r\n", game_state.score);
    UART_Printf("Obstacles Passed: %lu\r\n", Obstacles_CheckPassed(game_state.player_pos.z));
    UART_Printf("Press R to restart\r\n\r\n");
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
        // Wrap render with frame start/end so the new protocol receives frames
        SPI_MarkFrameStart();
        Render_Frame();
        SPI_MarkFrameEnd();
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
    // New protocol expects a rotation matrix; pass identity if none
    float identity_rot[9] = {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };
    {
        float player_pos_arr[3] = { game_state.player_pos.x, game_state.player_pos.y, game_state.player_pos.z };
        SPI_AddModelInstance(SHAPE_ID_PLAYER, player_pos_arr, identity_rot);
    }

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
                float rel_pos_arr[3] = { relative_pos.x, relative_pos.y, relative_pos.z };
                SPI_AddModelInstance(obstacles[i].shape_id, rel_pos_arr, identity_rot);
                instances_sent++;
            }
        }
    }

    // 3. Tell FPGA to render the frame
    // Note: frame boundaries are handled by Game_Update (SPI_MarkFrameStart/End)
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
