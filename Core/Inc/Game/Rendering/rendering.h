#ifndef INC_GAME_RENDERING_RENDERING_H_
#define INC_GAME_RENDERING_RENDERING_H_

#include "../game_types.h"
#include "stm32u5xx_hal.h"

// Renderer initialization
void Renderer_Init(SPI_HandleTypeDef* hspi);
void Renderer_UploadShapes(void);

// Rendering functions
void Renderer_DrawFrame(GameState* state);
void Renderer_ClearScene(void);
void Renderer_SendCollisionEffect(void);



#endif /* INC_GAME_RENDERING_RENDERING_H_ */
