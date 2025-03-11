#pragma once
#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include <stdbool.h>

#include "grid.h"
#include "labyrinth.h"

#define BUTTON_HINT		(1 << 0)  // 0b00000001
#define BUTTON_UP		(1 << 1)  // 0b00000010
#define BUTTON_ESC		(1 << 2)  // 0b00000100
#define BUTTON_LEFT		(1 << 3)  // 0b00001000
#define BUTTON_DOWN		(1 << 4)  // 0b00010000
#define BUTTON_RIGHT	(1 << 5)  // 0b00100000

typedef struct Player {
	int32_t x;
	int32_t y;
	uint32_t vx;
	uint32_t vy;
	uint32_t size;
	bool collision;

	float angle; // Radians

	grid_t grid_pos;
} player_t;


void player_move(player_t *player, char user_input, uint32_t button_val, maze_t *maze, uint16_t end_x, uint16_t end_y);
void player_draw(player_t *player, uint8_t *framebuf);
bool player_collision(player_t *player, maze_t *maze, uint16_t end_x, uint16_t end_y);
grid_t player_grid_position(player_t *player, maze_t *maze);

#endif
