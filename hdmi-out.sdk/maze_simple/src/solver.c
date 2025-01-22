
#include "solver.h"
#include "stdio.h"
#include "stdlib.h"
typedef struct miniMaze {
	uint16_t endX;
	uint16_t endY;
	uint16_t width;
	uint16_t height;
	uint8_t* data;
} miniMaze_t;

typedef struct linkerList {
	point_t* point;
	struct linkerList* next;
} linkerList_t;

typedef struct deletions {
	void* data;
	struct deletions* next;
} deletions_t;

void clean (deletions_t* delete, deletions_t* safe);

/**
	* checks if a point has been accessed before,
	* then if it is a path
	* @param current the point being checked
	* @return
	*/
uint8_t isAvailable (point_t* current, miniMaze_t* solverMaze) {
	if(current->X >= solverMaze->width || current->Y >= solverMaze->height) return 0;
	return mazeArray (solverMaze, current->X, current->Y);
}

void* alloc (size_t bytes,deletions_t* data) {
	void* out = malloc (bytes);
	deletions_t* temp = malloc (sizeof (deletions_t));
	if(out == NULL || temp == NULL) {
		if(out != NULL) free (out);
		if(temp!= NULL) free (temp);
		return NULL;
	}
	temp->data = out;
	deletions_t* temp2 = data->next;
	data->next = temp;
	temp->next = temp2;
	return out;
}

point_t* solve (maze_t* maze, uint16_t endX, uint16_t endY, uint8_t endOnSuccess) {
	deletions_t* deletions = malloc(sizeof(deletions_t));
	if(deletions == NULL) return NULL;
	deletions->next = NULL;
	deletions->data = NULL;
	deletions_t* safe = NULL;
	
	miniMaze_t* solverMaze = alloc (sizeof (miniMaze_t), deletions);
	if(solverMaze == NULL) {
		return NULL;
	}
	solverMaze->width = maze->width;
	solverMaze->height = maze->height;
	solverMaze->endX = endX;
	solverMaze->endY = endY;
	solverMaze->data = alloc (solverMaze->width * solverMaze->height, deletions);
	if(solverMaze->data == NULL) {
		free (solverMaze);
		return NULL;
	}
	for(uint16_t y = 0; y < solverMaze->height; y++) {
		for(uint16_t x = 0; x < solverMaze->width; x++) {
			mazeArray (solverMaze, x, y) = isPath (maze, x, y);
		}
	}

	linkerList_t* head = alloc (sizeof (linkerList_t),deletions);
	head->next = NULL;
	head->point = alloc (sizeof (point_t),deletions);
	head->point->X = maze->startX;
	head->point->Y = maze->startY;
	head->point->recursivePoint = NULL;
	linkerList_t* tail = head;

	point_t* succes = NULL;

	while(head != NULL) {
		point_t* current = head->point;
		if(isAvailable (current, solverMaze)) {
			mazeArray (solverMaze, current->X, current->Y) = 0;
			if(current->X == endX && current->Y == endY && safe == NULL) {
				point_t* temp = current;
				succes = current;
				while(temp != NULL) {
					deletions_t* temp2 = malloc (sizeof (deletions_t));
					if(temp2 == NULL) {
						clean (deletions, safe);
						return NULL;
					}
					temp2->data = temp;
					temp2->next = safe;
					safe = temp2;
					temp = temp->recursivePoint;
				}
				if(endOnSuccess == 0) {
					clean (deletions, safe);
					return current;
				}
			}
			for(int i = 0; i < 4; i++) {
				tail->next = alloc (sizeof (linkerList_t), deletions);
				tail = tail->next;
				tail->next = NULL;
				tail->point = alloc (sizeof (point_t), deletions);
				tail->point->recursivePoint = current;
				switch(i) {
				case 0:
					tail->point->X = current->X + 1;
					tail->point->Y = current->Y;
					break;
				case 1:
					tail->point->X = current->X - 1;
					tail->point->Y = current->Y;
					break;
				case 2:
					tail->point->X = current->X;
					tail->point->Y = current->Y + 1;
					break;
				case 3:
					tail->point->X = current->X;
					tail->point->Y = current->Y - 1;
					break;
				}
			}
		}
		head = head->next;
	}
	clean (deletions, safe);
	return succes;
}

void clean (deletions_t* delete, deletions_t* safe) {
	if(delete == NULL);
	while(delete != NULL) {
		deletions_t* secure = safe;
		uint8_t clearable = 1;
		while(secure != NULL) {
			if(secure->data == delete->data) {
				clearable = 0;
				break;
			}
			secure = secure->next;
		}
		if(clearable && delete->data != NULL) {
			free (delete->data);
		}
		deletions_t* temp = delete;
		delete = delete->next;
		free (temp);
	}
	while(safe != NULL) {
		deletions_t* temp = safe;
		safe = safe->next;
		free (temp);
	}
}
/*
int main () {
	maze_t* maze = NULL;
	int seed = 0;
	for(int i = 0; i < 1000000; i++) {
		while(1) {
			srand (seed++);
			maze = BuildMaze (20, 20, 0, 10, 0, 0);
			uint8_t out = labyrinthTest (maze, 19, 19);
			if(out == 0 || out == 0b100) {
				break;
			}
			free (maze->data);
			free (maze);
		}
		point_t* point = solve (maze, 19, 19, 1);
		if(point == NULL) {
			printf ("No solution found\n");
		} else {
			printf ("%d\r", i);
			//printf ("Solution found\n");
		}
		while(point != NULL) {
			point_t* temp = point;
			point = point->recursivePoint;
			free (temp);
		}
		free (maze->data);
		free (maze);
	}
	printf ("\n\n");
}
*/
