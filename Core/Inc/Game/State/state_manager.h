#ifndef INC_GAME_STATE_STATE_MANAGER_H_
#define INC_GAME_STATE_STATE_MANAGER_H_

#include "../game_types.h"

// State management functions
void StateManager_Init(GameState* state);
void StateManager_Reset(void);
void StateManager_TransitionTo(GameStateEnum new_state);
void StateManager_Pause(void);
void StateManager_Resume(void);
void StateManager_GameOver(void);

// State queries
GameStateEnum StateManager_GetCurrent(void);
const char* StateManager_GetStateName(GameStateEnum state);


#endif /* INC_GAME_STATE_STATE_MANAGER_H_ */
