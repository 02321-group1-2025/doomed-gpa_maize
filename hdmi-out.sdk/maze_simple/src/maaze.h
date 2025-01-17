#pragma once
#ifndef MAAZE_H
#define MAAZE_H

#include "stdlib.h"
#include "stdio.h"
#include "stdint.h"
//#include "time.h"

#define Debug 1

typedef struct mazeData {
    uint16_t width;
    uint16_t height;
    uint32_t start;
    uint16_t* data;
}maze_t;

#define mazeArray(maze,x,y) maze->data [y * maze->width + x]

int isPath (maze_t* maze, uint16_t x, uint16_t y);

maze_t* BuildMaze (uint16_t width, uint16_t height, uint8_t density, uint8_t scramble, uint16_t startX, uint16_t startY);
uint8_t labyrinthTest (maze_t* maze, uint16_t endX, uint16_t endY);
void printMaze (maze_t* maze);
void updateMaze (maze_t* maze);

#endif /* MAZE_H */
