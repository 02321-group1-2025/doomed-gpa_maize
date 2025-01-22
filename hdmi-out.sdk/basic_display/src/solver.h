#pragma once
#include "stdint.h"
#include "labyrinth.h"
typedef struct Point {
    uint16_t X;
    uint16_t Y;
    struct Point* recursivePoint;
} point_t;
point_t* solve (maze_t* maze, uint16_t endX, uint16_t endY, uint8_t endOnSucces);