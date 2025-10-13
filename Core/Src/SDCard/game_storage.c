#include "./SDCard/game_storage.h"
#include <string.h>

extern void UART_Printf(const char* format, ...);

#define SAVE_BLOCK 100  // Block address for save data
#define MAGIC_NUMBER 0x47414D45  // "GAME"

static uint8_t buffer[512];

SDResult Storage_Init(SPI_HandleTypeDef* hspi) {
    return SD_Init(hspi);
}

SDResult Storage_SaveGame(const GameSave* save) {
    memset(buffer, 0, 512);
    memcpy(buffer, save, sizeof(GameSave));

    return SD_WriteBlock(SAVE_BLOCK, buffer);
}

SDResult Storage_LoadGame(GameSave* save) {
    SDResult result = SD_ReadBlock(SAVE_BLOCK, buffer);
    if(result != SD_OK) return result;

    memcpy(save, buffer, sizeof(GameSave));

    // Validate
    if(save->magic != MAGIC_NUMBER) {
        // Initialize new save
        save->magic = MAGIC_NUMBER;
        save->version = 1;
        save->high_score = 0;
        save->total_played = 0;
        save->total_time = 0;
        Storage_SaveGame(save);
    }

    return SD_OK;
}


static uint32_t Get_Shape_Block(uint32_t shape_id) {
    switch(shape_id) {
        case SHAPE_PLAYER: return SHAPE_PLAYER_BLOCK;
        case SHAPE_CUBE:   return SHAPE_CUBE_BLOCK;
        case SHAPE_CONE:   return SHAPE_CONE_BLOCK;
        default: return 0;
    }
}

// Calculate checksum for shape data
static uint32_t Calculate_Shape_Checksum(const Shape3D* shape) {
    uint32_t checksum = 0;

    // XOR all vertex data
    for(int i = 0; i < shape->vertex_count; i++) {
        checksum ^= (uint32_t)(shape->vertices[i].x);
        checksum ^= (uint32_t)(shape->vertices[i].y);
        checksum ^= (uint32_t)(shape->vertices[i].z);
    }

    // XOR triangle data
    for(int i = 0; i < shape->triangle_count; i++) {
        checksum ^= shape->triangles[i].v1;
        checksum ^= shape->triangles[i].v2;
        checksum ^= shape->triangles[i].v3;
    }

    return checksum;
}

// Save a shape to SD card
SDResult Storage_SaveShape(uint32_t shape_id, const Shape3D* shape) {
    uint32_t block = Get_Shape_Block(shape_id);
    if(block == 0) return SD_ERROR;

    // Clear buffer
    memset(buffer, 0, 512);

    // Create header
    ShapeHeader* header = (ShapeHeader*)buffer;
    header->magic = SHAPE_MAGIC;
    header->version = 1;
    header->shape_id = shape_id;
    header->vertex_count = shape->vertex_count;
    header->triangle_count = shape->triangle_count;
    header->checksum = Calculate_Shape_Checksum(shape);

    // Calculate data size
    size_t vertex_size = shape->vertex_count * sizeof(Vertex3D);
    size_t triangle_size = shape->triangle_count * sizeof(Triangle);
    size_t total_size = vertex_size + triangle_size;

    // Check if it fits in one block (with header)
    if(total_size > 488) {  // 512 - sizeof(header basics)
        UART_Printf("Shape %lu too large for single block!\r\n", shape_id);
        // For larger shapes, we'd need multi-block storage
        return SD_ERROR;
    }

    // Copy shape data to buffer
    uint8_t* data_ptr = header->data;
    memcpy(data_ptr, shape->vertices, vertex_size);
    memcpy(data_ptr + vertex_size, shape->triangles, triangle_size);

    // Write to SD card
    SDResult result = SD_WriteBlock(block, buffer);
    if(result == SD_OK) {
        UART_Printf("Shape %lu saved to block %lu\r\n", shape_id, block);
    } else {
        UART_Printf("Failed to save shape %lu\r\n", shape_id);
    }

    return result;
}

// Load a shape from SD card
SDResult Storage_LoadShape(uint32_t shape_id, Shape3D* shape) {
    uint32_t block = Get_Shape_Block(shape_id);
    if(block == 0) return SD_ERROR;

    // Read from SD card
    SDResult result = SD_ReadBlock(block, buffer);
    if(result != SD_OK) {
        UART_Printf("Failed to read shape %lu from block %lu\r\n", shape_id, block);
        return result;
    }

    // Parse header
    ShapeHeader* header = (ShapeHeader*)buffer;

    // Validate magic number
    if(header->magic != SHAPE_MAGIC) {
        UART_Printf("Shape %lu not found on SD (invalid magic)\r\n", shape_id);
        return SD_ERROR;
    }

    // Validate shape ID matches
    if(header->shape_id != shape_id) {
        UART_Printf("Shape ID mismatch!\r\n");
        return SD_ERROR;
    }

    // Set shape metadata
    shape->id = shape_id;
    shape->vertex_count = header->vertex_count;
    shape->triangle_count = header->triangle_count;

    // Load vertex and triangle data
    size_t vertex_size = header->vertex_count * sizeof(Vertex3D);
    size_t triangle_size = header->triangle_count * sizeof(Triangle);

    uint8_t* data_ptr = header->data;
    memcpy(shape->vertices, data_ptr, vertex_size);
    memcpy(shape->triangles, data_ptr + vertex_size, triangle_size);

    // Verify checksum
    uint32_t calculated_checksum = Calculate_Shape_Checksum(shape);
    if(calculated_checksum != header->checksum) {
        UART_Printf("Shape %lu checksum mismatch! Data may be corrupted\r\n", shape_id);
        // Still return OK to use the data, but warn about corruption
    }

    UART_Printf("Shape %lu loaded from SD (v=%lu, t=%lu)\r\n",
                shape_id, header->vertex_count, header->triangle_count);

    return SD_OK;
}

// Check if a shape exists on SD card
uint8_t Storage_ShapeExists(uint32_t shape_id) {
    uint32_t block = Get_Shape_Block(shape_id);
    if(block == 0) return 0;

    // Read header only
    if(SD_ReadBlock(block, buffer) != SD_OK) {
        return 0;
    }

    ShapeHeader* header = (ShapeHeader*)buffer;
    return (header->magic == SHAPE_MAGIC && header->shape_id == shape_id);
}

// Initialize shapes - load from SD or save if not present
void Storage_InitializeShapes(void) {
    UART_Printf("Initializing shape storage...\r\n");

    if(!SD_IsPresent()) {
        UART_Printf("No SD card - using built-in shapes\r\n");
        return;
    }

    // Check each shape
    Shape3D* shapes[] = {
        Shapes_GetPlayer(),
        Shapes_GetCube(),
        Shapes_GetCone()
    };

    uint32_t shape_ids[] = {
        SHAPE_PLAYER,
        SHAPE_CUBE,
        SHAPE_CONE
    };

    const char* shape_names[] = {
        "Player",
        "Cube",
        "Cone"
    };

    for(int i = 0; i < 3; i++) {
        if(Storage_ShapeExists(shape_ids[i])) {
            // Load from SD card
            UART_Printf("Loading %s shape from SD...\r\n", shape_names[i]);
            if(Storage_LoadShape(shape_ids[i], shapes[i]) != SD_OK) {
                UART_Printf("Failed to load %s, using built-in\r\n", shape_names[i]);
            }
        } else {
            // Save built-in shape to SD
            UART_Printf("Saving %s shape to SD...\r\n", shape_names[i]);
            if(Storage_SaveShape(shape_ids[i], shapes[i]) != SD_OK) {
                UART_Printf("Failed to save %s shape\r\n", shape_names[i]);
            }
        }
    }

    UART_Printf("Shape storage initialization complete\r\n");
}


void Storage_Test(void) {
    UART_Printf("\r\n=== SD Card Test ===\r\n");

    // Test write
    memset(buffer, 0xAA, 512);
    if(SD_WriteBlock(10, buffer) == SD_OK) {
        UART_Printf("Write test: OK\r\n");
    }

    // Test read
    memset(buffer, 0, 512);
    if(SD_ReadBlock(10, buffer) == SD_OK) {
        UART_Printf("Read test: OK (First byte: 0x%02X)\r\n", buffer[0]);
    }
}
