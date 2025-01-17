#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
//#include "time.h"
#include "xil_printf.h"

#include "maaze.h"

//#define Debug 1
#define printf xil_printf

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

#define isValid(maze,x,y)  (x < maze->width && y < maze->height && x >= 0 && y >= 0)

uint16_t getX (uint32_t point) {
	return point >> 16;
}
uint16_t getY (uint32_t point) {
	return point & 0xFFFF;
}
uint32_t makePoint (uint16_t x, uint16_t y) {
	return (((uint32_t) x) << 16) | (uint32_t) y;
}
#define mazeArray(maze,x,y) maze->data [y * maze->width + x]

int isPath (maze_t* maze, uint16_t x, uint16_t y) {
    if(isValid (maze, x, y)) {
        return mazeArray(maze,x,y) != 0;
    } else {
		return 0;
    }
}
int setPath (maze_t* maze, uint16_t x, uint16_t y, uint8_t path) {
    if(isValid (maze, x, y) && !isPath(maze,x,y)) {
		mazeArray (maze, x, y) = path;
		return 1;
	} else {
		return 0;
	}
	return 0;
}

/**
 * check if a specific point should become a path
 * @param maze the mace being modified
 * @param p the point to be checked
 * @param density chances the chance of a path is placed, even if there are more than one path connected
 * @param scramble sets a chance a path that would otherwise have been places is not placed
 * @param ensurance make sure that i path is not blocked when no other is availble
 * @return new points to be checked
 */
quadPoint_t* modelMaze (maze_t* maze, uint16_t x, uint16_t y, uint8_t density, uint8_t scramble, uint16_t ensurance) {
    #ifdef debug
	    updateMaze (maze);
    #endif
    if(!isValid (maze, x, y)) return NULL;
    uint8_t paths = 0;
    paths += isPath (maze, x + 1, y);
    paths += isPath (maze, x - 1, y);
    paths += isPath (maze, x,     y + 1);
    paths += isPath (maze, x,     y - 1);

    if((density < 1 || (rand () % (density * 5)) != 0) && (paths > 1)) return NULL;

    if((rand () % (scramble)) == 0 && !ensurance) return NULL;

    if(!setPath (maze, x, y, 1) && !ensurance) return NULL;

    quadPoint_t* a = malloc (sizeof (quadPoint_t));
	if(a == NULL) return NULL;
    a->a = (((uint32_t) x) << 16)     |((uint32_t) y + 1);
    a->b = (((uint32_t) x + 1) << 16) | (uint32_t) y;
    a->c = (((uint32_t) x - 1) << 16) | (uint32_t) y;
    a->d = (((uint32_t) x) << 16) | ((uint32_t) y - 1);
    return a;
}

/**
 * shuffles an linkedList to make sure the order that points are added down not effekt the order in which they are used
 * @param active the points to be shuffled
 * @return shuffled points
 */
void shuffle (linkedList_t* list) {
    //srand (1);
    uint32_t length = 0;
    linkedList_t* tempArr = list;
    uint32_t index = 0;
    while(tempArr != NULL) {
        tempArr = tempArr->next;
        length++;
    }
    uint32_t* data = malloc (sizeof (uint32_t) * length);
    if(data == NULL) return;
    tempArr = list;
    while(tempArr != NULL && index < length) {//make array of data
        data [index] = tempArr->data;
        tempArr = tempArr->next;
        index++;
    }
    for(uint8_t i = 0; i < 4; i++) {//shuffle
        for(uint32_t j = 0; j < length; j++) {
            uint32_t num = rand () % length;
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

void freeList (linkedList_t* list) {
    while(list != NULL) {
        linkedList_t* temp = list;
        list = list->next;
        free (temp);
    }
}

/*
* @param width the number of points the labyrinth should be wide
* @param height the number of points the labyrinth should be tall
* @param density alows loop and the likelyhood of those loops
* @param scramble controlls the likelyhood of a point not getting a tile even if it is otherwise supposed to (chance of not placing 1/scramble)
* @return a newly generated labyrinth
*/
maze_t* BuildMaze (uint16_t width, uint16_t height, uint8_t density, uint8_t scramble, uint16_t startX, uint16_t startY) {
    maze_t* maze = malloc (sizeof (maze_t));
    if(maze == NULL) return NULL;
    maze->width = width;
    maze->height = height;
    maze->data = calloc (sizeof (uint16_t), height * width);
    if(maze->data == NULL) {
        free (maze);
        return NULL;
    }
	maze->start = makePoint (startX, startY);
    setPath (maze, startX, startY, 1);

    linkedList_t* OldPoints = malloc (sizeof (linkedList_t));
    if(OldPoints == NULL) {
        free (maze->data);
        free (maze);
        return NULL;
    }
    #ifdef debug
	    printMaze (maze);
    #endif
    OldPoints->data = (startX << 16) | startY;
    OldPoints->next = NULL;
    while(1) {
        linkedList_t* NewPoints = NULL;
        while(OldPoints != NULL) {
            /*if(rand () % 255 > 25) {//95 % chance to skip
                break;
            }*/
            linkedList_t* temp = OldPoints;
            OldPoints = OldPoints->next;
            uint16_t x = temp->data >> 16;
            uint16_t y = temp->data & 0xFFFF;
            free (temp);
            quadPoint_t* points = modelMaze (maze, x, y, density, scramble, OldPoints == NULL);
            if(points != NULL) {//save the new points
                linkedList_t* temp = malloc (sizeof (linkedList_t));
                if(temp == NULL) {
                    freeList (OldPoints);
                    freeList (NewPoints);
                    free (maze->data);
                    free (maze);
                    return NULL;
                }
                temp->data = points->a;
                temp->next = NewPoints;
                NewPoints = temp;
                temp = malloc (sizeof (linkedList_t));
                if(temp == NULL) {
                    freeList (OldPoints);
                    freeList (NewPoints);
                    free (maze->data);
                    free (maze);
                    return NULL;
                }
                temp->data = points->b;
                temp->next = NewPoints;
                NewPoints = temp;
                temp = malloc (sizeof (linkedList_t));
                if(temp == NULL) {
                    freeList (OldPoints);
                    freeList (NewPoints);
                    free (maze->data);
                    free (maze);
                    return NULL;
                }
                temp->data = points->c;
                temp->next = NewPoints;
                NewPoints = temp;
                temp = malloc (sizeof (linkedList_t));
                if(temp == NULL) {
                    freeList (OldPoints);
                    freeList (NewPoints);
                    free (maze->data);
                    free (maze);
                    return NULL;
                }
                temp->data = points->d;
                temp->next = NewPoints;
                NewPoints = temp;
                free (points);
            }
        }
        while(NewPoints != NULL) {//add the new points to the OldPoints
            linkedList_t* temp = malloc (sizeof (linkedList_t));
            if(temp == NULL) {
                freeList (OldPoints);
                freeList (NewPoints);
                free (maze->data);
                free (maze);
                return NULL;
            }
            temp->data = NewPoints->data;
            temp->next = OldPoints;
            OldPoints = temp;
            temp = NewPoints;
            NewPoints = NewPoints->next;
            free (temp);
        }
        if(OldPoints == NULL) {
            break;
        }

        linkedList_t* tempArr = OldPoints;
        uint32_t length = 0;

        shuffle (OldPoints);
    }
	freeList (OldPoints);
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
uint8_t labyrinthTest (maze_t* maze, uint16_t endX, uint16_t endY) {
    int sum = 0;
    for(int x = 0; x < maze->width; x++) {
        for(int y = 0; y < maze->height; y++) {
            sum += isPath (maze, x, y);
        }
    }
    if((float) sum > (maze->width * maze->height) / 2) {
        if(!setPath (maze, endX, endY, 1)) {
            uint8_t found = 0;
            found += isPath (maze, endX + 1, endY);
            found += isPath (maze, endX - 1, endY);
            found += isPath (maze, endX, endY + 1);
            found += isPath (maze, endX, endY - 1);
            if(found == 0) {
                return 1;
            } else {
                return 0;
            }
        }
    }
    return 0;
}

/*
 @param maze the maze to be printed
*/
void printMaze (maze_t* maze) {
    for(size_t i = 0; i < maze->width+2; i++) {
        printf ("-");
    }printf ("\r\n");
    for(int y = 0; y < maze->height; y++) {
		printf ("|");
        for(int x = 0; x < maze->width; x++) {
            if(isPath (maze, x, y)) {
                printf ("#");
            } else {
                printf (" ");
            }
        }
		printf ("|");
        printf ("\r\n");
    }
    for(size_t i = 0; i < maze->width+2; i++) {
        printf ("-");
    }printf ("r\n");
}
/*
 @param maze moves the cursor to the start of a maze, must be called after a printmaze
*/
void updateMaze (maze_t* maze) {
    printf ("\033[%dA\r",maze->width);
	printMaze (maze);
}

//int main () {
//    srand ((unsigned) time (NULL));
//    maze_t* maze = BuildMaze (20, 20, 0, 100, 0, 0);
//    if(maze == NULL) {
//        printf ("Failed to build maze\n");
//        return;
//    }
//    printMaze (maze);
//	if(labyrinthTest (maze,19,19)) {
//		printf ("Labyrinth passed\n");
//    } else {
//		printf ("Labyrinth failed\n");
//	}
//}
