/*
 * shapes.c
 *
 *  Created on: Sep 15, 2025
 *      Author: jornik
 */

// shapes.c - 3D shape creation implementation
#include "./Game/shapes.h"
#include <math.h>
#include <string.h>

// Static shape storage (singleton pattern)
static Shape3D player_shape;
static Shape3D cube_shape;
static Shape3D cone_shape;
static Shape3D pyramid_shape;
static uint8_t shapes_initialized = 0;

// Initialize all shapes
void Shapes_Init(void)
{
    if(!shapes_initialized)
    {
        Shapes_CreatePlayer(&player_shape);
        Shapes_CreateCube(&cube_shape);
        Shapes_CreateCone(&cone_shape);
        Shapes_CreatePyramid(&pyramid_shape);
        shapes_initialized = 1;
    }
}

// Create player shape (paper airplane)
void Shapes_CreatePlayer(Shape3D* shape)
{
    memset(shape, 0, sizeof(Shape3D));

    shape->id = SHAPE_PLAYER;
    shape->vertex_count = 5;
    shape->triangle_count = 4;

    // Vertices for paper airplane
    shape->vertices[0] = (Vertex3D){0, 0, -5};      // Nose
    shape->vertices[1] = (Vertex3D){-2, 0, 0};    // Left wing tip
    shape->vertices[2] = (Vertex3D){2, 0, 0};     // Right wing tip
    shape->vertices[3] = (Vertex3D){0, 1, -1};     // Middle top

    // Triangles (two wings forming V shape)
    shape->triangles[0] = (Triangle){0, 1, 3};  // Left wing
    shape->triangles[1] = (Triangle){0, 2, 3};  // Center body
    shape->triangles[2] = (Triangle){1, 2, 3};  // Right wing
    // shape->triangles[3] = (Triangle){2, 1, 3};  // Back face

    // Calculate and store bounds
    Shapes_CalculateBounds(shape, &shape->width, &shape->height, &shape->depth);
}

// Create cube shape
void Shapes_CreateCube(Shape3D* shape)
{
    memset(shape, 0, sizeof(Shape3D));

    shape->id = SHAPE_CUBE;
    shape->vertex_count = 8;
    shape->triangle_count = 12;

    int16_t s = 2;  // Half-size

    // Define vertices
    shape->vertices[0] = (Vertex3D){-s, -s, -s};
    shape->vertices[1] = (Vertex3D){ s, -s, -s};
    shape->vertices[2] = (Vertex3D){ s,  s, -s};
    shape->vertices[3] = (Vertex3D){-s,  s, -s};
    shape->vertices[4] = (Vertex3D){-s, -s,  s};
    shape->vertices[5] = (Vertex3D){ s, -s,  s};
    shape->vertices[6] = (Vertex3D){ s,  s,  s};
    shape->vertices[7] = (Vertex3D){-s,  s,  s};

    // Define triangles (2 per face)
    // Front face
    shape->triangles[0] = (Triangle){4, 5, 6};
    shape->triangles[1] = (Triangle){4, 6, 7};
    // Back face
    shape->triangles[2] = (Triangle){1, 0, 3};
    shape->triangles[3] = (Triangle){1, 3, 2};
    // Top face
    shape->triangles[4] = (Triangle){7, 6, 2};
    shape->triangles[5] = (Triangle){7, 2, 3};
    // Bottom face
    shape->triangles[6] = (Triangle){0, 1, 5};
    shape->triangles[7] = (Triangle){0, 5, 4};
    // Right face
    shape->triangles[8] = (Triangle){5, 1, 2};
    shape->triangles[9] = (Triangle){5, 2, 6};
    // Left face
    shape->triangles[10] = (Triangle){0, 4, 7};
    shape->triangles[11] = (Triangle){0, 7, 3};

    // Calculate and store bounds
    Shapes_CalculateBounds(shape, &shape->width, &shape->height, &shape->depth);
}

// Create cone shape
void Shapes_CreateCone(Shape3D* shape)
{
    memset(shape, 0, sizeof(Shape3D));

    shape->id = SHAPE_CONE;
    shape->vertex_count = 9;
    shape->triangle_count = 8;

    // Apex at top
    shape->vertices[0] = (Vertex3D){0, 15, 0};

    // Base vertices (octagon)
    int16_t radius = 8;
    for(int i = 0; i < 8; i++)
    {
        float angle = (i * 2 * 3.14159f) / 8;
        shape->vertices[i+1] = (Vertex3D){
            (int16_t)(radius * cosf(angle)),
            -10,
            (int16_t)(radius * sinf(angle))
        };
    }

    // Create triangular faces from apex to base
    for(int i = 0; i < 8; i++)
    {
        int next = (i + 1) % 8;
        shape->triangles[i] = (Triangle){0, i+1, next+1};
    }

    // Calculate and store bounds
    Shapes_CalculateBounds(shape, &shape->width, &shape->height, &shape->depth);
}

// Create pyramid shape
void Shapes_CreatePyramid(Shape3D* shape)
{
    memset(shape, 0, sizeof(Shape3D));

    shape->id = SHAPE_PYRAMID;
    shape->vertex_count = 5;
    shape->triangle_count = 6;

    // Apex
    shape->vertices[0] = (Vertex3D){0, 12, 0};

    // Square base
    int16_t s = 10;
    shape->vertices[1] = (Vertex3D){-s, -8, -s};
    shape->vertices[2] = (Vertex3D){ s, -8, -s};
    shape->vertices[3] = (Vertex3D){ s, -8,  s};
    shape->vertices[4] = (Vertex3D){-s, -8,  s};

    // Side triangles
    shape->triangles[0] = (Triangle){0, 1, 2};
    shape->triangles[1] = (Triangle){0, 2, 3};
    shape->triangles[2] = (Triangle){0, 3, 4};
    shape->triangles[3] = (Triangle){0, 4, 1};
    // Base triangles
    shape->triangles[4] = (Triangle){1, 3, 2};
    shape->triangles[5] = (Triangle){1, 4, 3};

    // Calculate and store bounds
    Shapes_CalculateBounds(shape, &shape->width, &shape->height, &shape->depth);
}

// Scale a shape by a factor
void Shapes_Scale(Shape3D* shape, float scale)
{
    for(int i = 0; i < shape->vertex_count; i++)
    {
        shape->vertices[i].x = (int16_t)(shape->vertices[i].x * scale);
        shape->vertices[i].y = (int16_t)(shape->vertices[i].y * scale);
        shape->vertices[i].z = (int16_t)(shape->vertices[i].z * scale);
    }
    // Recalculate bounds after scaling
    Shapes_CalculateBounds(shape, &shape->width, &shape->height, &shape->depth);
}

// Calculate bounding box dimensions
void Shapes_CalculateBounds(Shape3D* shape, float* width, float* height, float* depth)
{
    if(shape->vertex_count == 0) return;

    int16_t min_x = shape->vertices[0].x, max_x = shape->vertices[0].x;
    int16_t min_y = shape->vertices[0].y, max_y = shape->vertices[0].y;
    int16_t min_z = shape->vertices[0].z, max_z = shape->vertices[0].z;

    for(int i = 1; i < shape->vertex_count; i++)
    {
        if(shape->vertices[i].x < min_x) min_x = shape->vertices[i].x;
        if(shape->vertices[i].x > max_x) max_x = shape->vertices[i].x;
        if(shape->vertices[i].y < min_y) min_y = shape->vertices[i].y;
        if(shape->vertices[i].y > max_y) max_y = shape->vertices[i].y;
        if(shape->vertices[i].z < min_z) min_z = shape->vertices[i].z;
        if(shape->vertices[i].z > max_z) max_z = shape->vertices[i].z;
    }

    *width = (float)(max_x - min_x);
    *height = (float)(max_y - min_y);
    *depth = (float)(max_z - min_z);
}

// Get singleton instances
Shape3D* Shapes_GetPlayer(void)
{
    if(!shapes_initialized) Shapes_Init();
    return &player_shape;
}

Shape3D* Shapes_GetCube(void)
{
    if(!shapes_initialized) Shapes_Init();
    return &cube_shape;
}

Shape3D* Shapes_GetCone(void)
{
    if(!shapes_initialized) Shapes_Init();
    return &cone_shape;
}

Shape3D* Shapes_GetPyramid(void)
{
    if(!shapes_initialized) Shapes_Init();
    return &pyramid_shape;
}
