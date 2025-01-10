#include "stdlib.h"
#include "stdio.h"
#include "stdint.h"
#include "math.h"

typedef struct mazeData {
    uint16_t width;
    uint16_t height;
    uint32_t* start;
    uint16_t* data;
}maze_t;

typedef struct linkedList {
    uint32_t data;
    struct linkedList* next;
}linkedList_t;

typedef struct quadPoint {
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
}quadPoint_t;

#define isValid(maze,x,y)  (x < 0 || y < 0 || x >= maze->width || y >= maze->height)
#define isPath(maze, x, y) isValid(maze,x,y) ? 0 : maze->data [y * maze->width + x]
#define setPath(maze, x, y, path) if(isValid(maze,x,y)){maze->data [y * maze->width + x] = path;}

/**
 * check if a specific point should become a path
 * @param maze the mace being modified
 * @param p the point to be checked
 * @param density chances the chance of a path is placed, even if there are more than one path connected
 * @param scramble sets a chance a path that would otherwise have been places is not placed
 * @return new points to be checked
 */
quadPoint_t* modelMaze (maze_t* maze, uint16_t x, uint16_t y, uint8_t density, uint8_t scramble) {
    if(isValid (maze, x, y)) return NULL;
    uint8_t paths = 0;
    paths += isPath (maze, x + 1, y);
    paths += isPath (maze, x - 1, y);
    paths += isPath (maze, x, y + 1);
    paths += isPath (maze, x, y - 1);

    if((density < 1 || (rand () % (density * 5)) != 0) && (paths > 1)) return NULL;

    if((rand () % (scramble)) <= 0) return NULL;

    setPath (maze, x, y + 1, 1);

    quadPoint_t* a = malloc (sizeof (quadPoint_t));
    a->a = ((x) << 16) | (y + 1);
    a->b = ((x + 1) << 16) | y;
    a->c = ((x - 1) << 16) | y;
    a->d = ((x) << 16) | (y - 1);
    return a;
}

/**
 * shuffles an linkedList to make sure the order that points are added down not effekt the order in which they are used
 * @param active the points to be shuffled
 * @return shuffled points
 */
void shuffle (linkedList_t* list, uint16_t length) {
    uint32_t* data = malloc (sizeof (uint32_t) * length);
    linkedList_t* tempArr = list;
    uint32_t index = 0;
    while(tempArr != NULL) {//make array of data
        data [index] = tempArr->data;
        tempArr = tempArr->next;
    }
    for(uint8_t i = 0; i < 4; i++) {//shuffle
        for(uint16_t j = 0; j < length; j++) {
            uint16_t num = rand () % length;
            uint32_t tempData = data [j];
            data [j] = data [num];
            data [num] = tempData;
        }
    }
    tempArr = list;
    index = 0;
    while(tempArr != NULL) {//back to list
        tempArr->data = data [index];
        tempArr = tempArr->next;
        index++;
    }
}

/*
* @param width the number of points the labyrinth should be wide
* @param height the number of points the labyrinth should be tall
* @param density the parameter that controlls wether or not and how likely a point is to skip the path check
* @param scramble controlls the likelyhood of a point not getting a tile even if it is otherwise supposed to
* @return a newly generated labyrinth
*/
maze_t* BuildMaze (uint16_t width, uint16_t height, uint8_t density, uint8_t scramble, uint16_t startX, uint16_t startY) {
    maze_t* maze = malloc (sizeof (maze_t));
    maze->width = width;
    maze->height = height;
    maze->data = calloc (sizeof (uint16_t), height * width);
    maze->start = (startX << 16) | startY;

    linkedList_t* OldPoints = malloc (sizeof (linkedList_t));
    OldPoints->data = (startX << 16) | startY;
    maze->data [startY * width + startX] = 1;
    uint16_t length = 1;
    while(1) {
        linkedList_t* NewPoints = NULL;
        while(OldPoints != NULL) {
            if(rand () % 255 > 242) {//95 % chance to skip
                break;
            }
            linkedList_t* temp = OldPoints;
            OldPoints = OldPoints->next;
            uint16_t x = temp->data >> 16;
            uint16_t y = temp->data & 0xFFFF;
            free (temp);
            length--;
            quadPoint_t* points = modelMaze (maze, x, y, density, scramble);
            if(points != NULL) {//save the new points
                linkedList_t* temp = malloc (sizeof (linkedList_t));
                temp->data = points->a;
                temp->next = NewPoints;
                NewPoints = temp;
                linkedList_t* temp = malloc (sizeof (linkedList_t));
                temp->data = points->b;
                temp->next = NewPoints;
                NewPoints = temp;
                temp = malloc (sizeof (linkedList_t));
                temp->data = points->c;
                temp->next = NewPoints;
                NewPoints = temp;
                temp = malloc (sizeof (linkedList_t));
                temp->data = points->d;
                temp->next = NewPoints;
                NewPoints = temp;
                free (points);
            }
        }
        while(NewPoints != NULL) {//add the new points to the OldPoints
            linkedList_t* temp = malloc (sizeof (linkedList_t));
            temp->data = NewPoints->data;
            temp->next = OldPoints;
            OldPoints = temp;
            temp = NewPoints;
            NewPoints = NewPoints->next;
            free (temp);
            length++;
        }
        if(OldPoints == NULL) {
            break;
        }
        shuffle (OldPoints, length);
    }
    return maze;
}

/**
 * test if a maze has enough tiles to be considered good
 * @param maze the labyrinth to be tested
 * @param width the width of the labyrinth (in case it failes)
 * @param height the height of the labyrinth (in case it failes)
 * @param density the density of the labyrinth (in case it failes)
 * @param scramble the scramble of the labyrinth (in case it failes)
 * @return a passing labyrinth
 */
uint8_t labyrinthTest (maze_t* maze) {
    int sum = 0;
    for(int x = 0; x < maze->width; x++) {
        for(int y = 0; y < maze->height; y++) {
			sum += isPath (maze, x, y);
        }
    }
    if((float) sum > (maze->width * maze->height) * 3 / 4) {
        return 1;
    }
    return 0;
}
