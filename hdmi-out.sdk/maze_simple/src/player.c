#include <stdbool.h>
#include <math.h>

#include "player.h"
#include "draw.h"
#include "display.h"
#include "grid.h"
#include "maze.h"

#include "xil_printf.h"

#define printf xil_printf

#define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

enum Direction {
	NORTH,
	EAST,
	SOUTH,
	WEST,
};

enum Direction player_cardinal_direction(player_t *player) {
	enum Direction direction;

	float player_angle = player->angle;

	float quadrant = (2*M_PI/4);

	if (player_angle > (M_PI/4) && player_angle < (3*M_PI/4)) direction = NORTH;
	if (player_angle >= (3*M_PI/4) && player_angle < (5*M_PI/4)) direction = WEST;

	return direction;
}

void
player_move(player_t *player, char user_input, uint32_t button_value, maze_t *maze, uint16_t end_x, uint16_t end_y) {
	int player_size = player->size;

	// Save player pos and grid pos if they collide with shit
	int player_x = player->x;
	int player_y = player->y;
	grid_t player_grid = player->grid_pos;

	enum Direction direction;

	switch (user_input) {
		case 'w': {
			int x_delta = 10 * cosf(player->angle);
			int y_delta = 10 * sinf(player->angle);

			player->x += x_delta;
			player->y += y_delta;

			//player->y -= 10;
			direction = NORTH;
			break;
		}
		case 'a': {
			//player->x -= 10;
			player->angle -= (M_PI/18);

			if (player->angle < 0) {
				player->angle += 2*M_PI;
			}

			direction = WEST;
			break;
		}
		case 's': {
			int x_delta = 10 * cosf(player->angle);
			int y_delta = 10 * sinf(player->angle);

			player->x -= x_delta;
			player->y -= y_delta;

			//player->y += 10;
			direction = SOUTH;
			break;
		}
		case 'd': {
			player->angle += (M_PI/18);

			if (player->angle > 2*M_PI) {
				player->angle -= 2*M_PI;
			}

			direction = EAST;
			break;
		}
	}

	// Buttons
	if (button_value & BUTTON_UP) {
		int x_delta = 5 * cosf(player->angle);
		int y_delta = 5 * sinf(player->angle);

		player->x += x_delta;
		player->y += y_delta;
    }

	if (button_value & BUTTON_LEFT) {
		player->angle -= (M_PI/36);

		if (player->angle < 0) {
			player->angle += 2*M_PI;
		}
	}

    if (button_value & BUTTON_DOWN) {
    	float x_delta = 5 * cosf(player->angle);
    	float y_delta = 5 * sinf(player->angle);

		player->x -= x_delta;
		player->y -= y_delta;
    }

    if (button_value & BUTTON_RIGHT) {
    	player->angle += (M_PI/36);

		if (player->angle > 2*M_PI) {
			player->angle -= 2*M_PI;
		}
    }


	player->grid_pos = player_grid_position(player, maze);


	bool player_does_collide = player_collision(player, maze, end_x, end_y);
	if (player_does_collide == true) { // TODO: Collision bad. Snap to wall plz
//		switch (direction) {
//			case NORTH: { // Up
//				player->y = player_grid.row * GRID_INTERVAL_Y;
//
//				break;
//			}
//			case EAST: { // Right
//				player->x = ((player_grid.col+1) * GRID_INTERVAL_X) - player->size;
//
//				break;
//			}
//			case SOUTH: { // South
//				player->y = ((player_grid.row+1) * GRID_INTERVAL_Y) - player->size;
//
//				break;
//			}
//			case WEST: { // Left
//				player->x = player_grid.col * GRID_INTERVAL_X;
//
//				break;
//			}
//		}
		player->x = player_x;
		player->y = player_y;
	}

}

extern const float FOV;


void
player_draw(player_t *player, uint8_t *framebuf) {
	rect(
			framebuf,
			0xA1, 0xBE, 0xD0,
			player->x - (player->size/2), player->y - (player->size/2),
			player->size, player->size,
			1);

	int x0 = player->x;
	int y0 = player->y;

	const int NUM_RAYS = DISPLAY_WIDTH * 2;
	const float ANGLE_STEP = FOV / NUM_RAYS;  // Angle between rays



	float start_angle = player->angle - FOV/2;

	// Draw fov lines
	for (int i = 0; i < NUM_RAYS; i++) {

		float ray_angle = start_angle + (i * ANGLE_STEP);

		int x_delta = 50 * cosf(ray_angle);
		int y_delta = 50 * sinf(ray_angle);

		int x1 = x0 + x_delta;
		int y1 = y0 + y_delta;


		line(framebuf, 0x00, 0xFF, 0x00, x0, y0, x1, y1);
	}


}

bool
player_collision(player_t *player, maze_t *maze, uint16_t end_x, uint16_t end_y) {
	if (player->collision == false) return false;

	grid_t player_grid_pos = player_grid_position(player, maze);

	// Don't collide with the end box
	if(player_grid_pos.col == end_x && player_grid_pos.row == end_y){
		return false;
	}


	if (!isPath(maze, player_grid_pos.col, player_grid_pos.row)) {
		return true;
	}

	return false;
}

grid_t
player_grid_position(player_t *player, maze_t *maze) {
	int col = (player->x / (DISPLAY_WIDTH / maze->width));
	int row = (player->y / (DISPLAY_HEIGHT / maze->height));

	return (grid_t) {col, row};
}
