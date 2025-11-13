#ifndef INC_SDCARD_GAME_STORAGE_H_
#define INC_SDCARD_GAME_STORAGE_H_

#include "./SDCard/sd_card.h"
#include "./Game/shapes.h"
#include "./Game/game_types.h"

// Save shapes used in game
typedef struct {
    uint32_t magic;           // 0x53485045 ("SHPE")
    uint32_t version;
    uint32_t shape_id;
    uint32_t vertex_count;
    uint32_t triangle_count;
    uint32_t checksum;
    uint8_t data[488];        // Remaining space in 512-byte block
} ShapeHeader;

#define SAVE_BLOCK          100  // Game save data
#define SHAPE_BASE_BLOCK    200  // Start of shape storage
#define SHAPE_PLAYER_BLOCK  200  // Player shape
#define SHAPE_CUBE_BLOCK    201  // Cube shape
#define SHAPE_CONE_BLOCK    202  // Cone shape
#define SHAPE_MAGIC         0x53485045  // "SHPE"

// Function prototypes
SDResult Storage_Init(SPI_HandleTypeDef* hspi);
SDResult Storage_SaveGame(const GameSave* save);
SDResult Storage_LoadGame(GameSave* save);
SDResult Storage_SaveHighScore(uint32_t score);
uint32_t Storage_GetHighScore(void);
void Storage_Test(void);

// Function prototypes of shape storage
SDResult Storage_SaveShape(uint32_t shape_id, const Shape3D* shape);
SDResult Storage_LoadShape(uint32_t shape_id, Shape3D* shape);
uint8_t Storage_ShapeExists(uint32_t shape_id);
void Storage_InitializeShapes(void);
#endif /* INC_SDCARD_GAME_STORAGE_H_ */
