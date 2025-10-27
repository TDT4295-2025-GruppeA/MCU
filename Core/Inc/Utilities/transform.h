#ifndef INC_UTILITIES_TRANSFORM_H_
#define INC_UTILITIES_TRANSFORM_H_

#include <math.h>

// 3x3 transformation matrix (row-major order)
typedef struct {
    float m[9];  // m[0-2] = row 1, m[3-5] = row 2, m[6-8] = row 3
} Matrix3x3;

// Create identity matrix
static inline void Matrix_Identity(Matrix3x3* mat) {
    mat->m[0] = 1.0f; mat->m[1] = 0.0f; mat->m[2] = 0.0f;
    mat->m[3] = 0.0f; mat->m[4] = 1.0f; mat->m[5] = 0.0f;
    mat->m[6] = 0.0f; mat->m[7] = 0.0f; mat->m[8] = 1.0f;
}

// Create rotation matrix around Y-axis (for turning objects)
static inline void Matrix_RotateY(Matrix3x3* mat, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);

    mat->m[0] = c;    mat->m[1] = 0.0f; mat->m[2] = s;
    mat->m[3] = 0.0f; mat->m[4] = 1.0f; mat->m[5] = 0.0f;
    mat->m[6] = -s;   mat->m[7] = 0.0f; mat->m[8] = c;
}

// Create rotation matrix around X-axis (for tilting)
static inline void Matrix_RotateX(Matrix3x3* mat, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);

    mat->m[0] = 1.0f; mat->m[1] = 0.0f; mat->m[2] = 0.0f;
    mat->m[3] = 0.0f; mat->m[4] = c;    mat->m[5] = -s;
    mat->m[6] = 0.0f; mat->m[7] = s;    mat->m[8] = c;
}

// Create rotation matrix around Z-axis (for rolling)
static inline void Matrix_RotateZ(Matrix3x3* mat, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);

    mat->m[0] = c;    mat->m[1] = -s;   mat->m[2] = 0.0f;
    mat->m[3] = s;    mat->m[4] = c;    mat->m[5] = 0.0f;
    mat->m[6] = 0.0f; mat->m[7] = 0.0f; mat->m[8] = 1.0f;
}

// Scale matrix (for size changes)
static inline void Matrix_Scale(Matrix3x3* mat, float sx, float sy, float sz) {
    mat->m[0] = sx;   mat->m[1] = 0.0f; mat->m[2] = 0.0f;
    mat->m[3] = 0.0f; mat->m[4] = sy;   mat->m[5] = 0.0f;
    mat->m[6] = 0.0f; mat->m[7] = 0.0f; mat->m[8] = sz;
}

#endif /* INC_UTILITIES_TRANSFORM_H_ */
