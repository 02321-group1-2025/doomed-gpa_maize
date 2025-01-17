#pragma once
#ifndef LABYRINTH_H
#define LABYRINTH_H
#include "stdint.h"

typedef struct mazeData {
    uint16_t width;
    uint16_t height;
    uint16_t startX;
    uint16_t startY;
    uint16_t* data;
}maze_t;

/**
 moves the cursor to the start of a maze, must be called after a printmaze
*/
void updateMaze (maze_t* maze);

/**
 @param maze the maze to be printed
*/
void printMaze (maze_t* maze);

/**
* @param maze the maze to be checked
* returns whether a path is present in the maze
*/
int isPath (maze_t* maze, uint16_t x, uint16_t y);

/**
 * test if a maze has enough tiles to be considered good
 * @param maze the labyrinth to be tested
 * @param width the width of the labyrinth (in case it failes)
 * @param height the height of the labyrinth (in case it failes)
 * @param density the density of the labyrinth (in case it failes)
 * @param scramble the scramble of the labyrinth (in case it failes)
 * @return a passing labyrinth
 */
uint8_t labyrinthTest (maze_t* maze, uint16_t endX, uint16_t endY);

/*
* @param width the number of points the labyrinth should be wide
* @param height the number of points the labyrinth should be tall
* @param density alows loop and the likelyhood of those loops
* @param scramble controlls the likelyhood of a point not getting a tile even if it is otherwise supposed to (chance of not placing 1/scramble)
* @return a newly generated labyrinth
*/
maze_t* BuildMaze (uint16_t width, uint16_t height, uint8_t density, uint8_t scramble, uint16_t startX, uint16_t startY);

/**
 * gives direct acces to a point in the maze
 */

#define mazeArray(maze,x,y) maze->data [y * maze->width + x]

#define makeColor(red,green,blue) (red&0b11111)<<10 | (green&0b11111)<<5 | (blue&0b11111)<<0;
#define setColor(maze,x,y,color) mazeArray(maze,x,y) |= (color & 0x7FFF);
#define setColors(maze,x,y,red,green,blue) mazeArray(maze,x,y) = (red&0b11111)<<10 | (green&0b11111)<<5 | (blue&0b11111)<<0;
#define setRed(maze,x,y,red)     mazeArray(maze,x,y) & (~(0b11111 << 10)) |= (red&0b11111)<<10;
#define setGreen(maze,x,y,green) mazeArray(maze,x,y) & (~(0b11111 <<  5)) |= (red&0b11111)<< 5;
#define setBlue(maze,x,y,blue)   mazeArray(maze,x,y) & (~(0b11111 <<  0)) |= (red&0b11111)<< 0;

#define getColor(maze,x,y) mazeArray(maze,x,y) & 0x7FFF;
#define getRed(color)      (color>>10)&0b11111;
#define getGreen(color)     (color>>5)&0b111111;
#define getBlue(color)           color&0b11111;

#endif
