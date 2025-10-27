#ifndef INC_GAME_LOGIC_GAME_LOGIC_H_
#define INC_GAME_LOGIC_GAME_LOGIC_H_

#include "../game_types.h"
#include <stdbool.h>

// Logic system initialization
void GameLogic_Init(void);

// Game mechanics
void GameLogic_Update(GameState* state, float delta_time);
void GameLogic_MovePlayer(GameState* state, float delta_x);
bool GameLogic_CheckCollisions(GameState* state);
void GameLogic_UpdateScore(GameState* state);



#endif /* INC_GAME_LOGIC_GAME_LOGIC_H_ */
