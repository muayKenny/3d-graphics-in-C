#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "display.h"
#include "texture.h"
#include <stdint.h>

#include "vector.h"

typedef struct {
    int a, b, c;
    color_t color;
    tex2_t a_uv;
    tex2_t b_uv;
    tex2_t c_uv;
} face_t;

typedef struct {
    vec2_t points[3];
    color_t color;
    tex2_t texcoords[3];
    float avg_depth;
} triangle_t;

void draw_filled_triangle(int x0, int y0, int x1, int y1, int x2, int y2,
                          color_t color);

//  TODO: void draw_textured_triangle

#endif