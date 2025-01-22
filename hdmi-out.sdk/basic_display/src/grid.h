#pragma once
#ifndef GRID_H
#define GRID_H

#include <stdint.h>

#include "display.h"

#define GRID_WIDTH  20
#define GRID_HEIGHT 20

#define GRID_INTERVAL_X (DISPLAY_WIDTH / GRID_WIDTH)
#define GRID_INTERVAL_Y (DISPLAY_HEIGHT / GRID_HEIGHT)

typedef struct Grid {
	uint32_t col;
	uint32_t row;
} grid_t;


#endif /* GRID_H */
