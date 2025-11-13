#include "../../../Inc/Game/Logic/game_logic.h"
#include "../../../Inc/Game/obstacles.h"
#include "../../../Inc/Game/collision.h"

extern void UART_Printf(const char* format, ...);

static uint32_t score_multiplier = 1;

void GameLogic_Init(void)
{
    score_multiplier = 1;
    UART_Printf("Game logic initialized\r\n");
}

void GameLogic_Update(GameState* state, float delta_time)
{
    if(!state || state->state != GAME_STATE_PLAYING) return;

    // Track distance "traveled"
    if(state->moving_forward) {
        float distance_moved = FORWARD_SPEED * delta_time;
        state->total_distance += distance_moved;
        // Move obstacles toward player
        Obstacles_MoveTowardPlayer(distance_moved);
    }

    // Update obstacles (spawning/despawning)
    Obstacles_Update(0, delta_time);
}

void GameLogic_MovePlayer(GameState* state, float delta_x)
{
    if(!state) return;

    state->player_pos.x += delta_x;

    // Clamp to world bounds
    if(state->player_pos.x < WORLD_MIN_X) {
        state->player_pos.x = WORLD_MIN_X;
    } else if(state->player_pos.x > WORLD_MAX_X) {
        state->player_pos.x = WORLD_MAX_X;
    }

    UART_Printf("Player X=%d\r\n", (int)state->player_pos.x);
}

bool GameLogic_CheckCollisions(GameState* state)
{
    if(!state) return false;

    Obstacle* obstacles = Obstacles_GetArray();
    CollisionResult collision = Collision_CheckPlayer(&state->player_pos,
                                                      obstacles, MAX_OBSTACLES);

    return (collision.type != COLLISION_NONE);
}

void GameLogic_UpdateScore(GameState* state)
{
    if(!state) return;

    uint32_t obstacles_passed = Obstacles_CheckPassed(0);
    state->score = (uint32_t)state->total_distance +
                   (obstacles_passed * 10 * score_multiplier);
}
