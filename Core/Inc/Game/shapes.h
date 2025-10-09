/*
 * shapes.h
 *
 *  Created on: Sep 15, 2025
 *      Author: jornik
 */

// shapes.h - 3D shape creation and management
#ifndef SHAPES_H
#define SHAPES_H

#include "game_types.h"
#define SHAPE_ID_PLAYER  SHAPE_PLAYER

// Shape creation functions
void Shapes_Init(void);
void Shapes_CreatePlayer(Shape3D* shape);
void Shapes_CreateCube(Shape3D* shape);
void Shapes_CreateCone(Shape3D* shape);
void Shapes_CreatePyramid(Shape3D* shape);

// Shape utility functions
void Shapes_Scale(Shape3D* shape, float scale);
void Shapes_CalculateBounds(Shape3D* shape, float* width, float* height, float* depth);

// Get pre-initialized shapes (singleton pattern)
Shape3D* Shapes_GetPlayer(void);
Shape3D* Shapes_GetCube(void);
Shape3D* Shapes_GetCone(void);

#endif // SHAPES_H
