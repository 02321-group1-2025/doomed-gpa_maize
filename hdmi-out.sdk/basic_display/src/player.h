#pragma once
#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include <stdbool.h>

#include "grid.h"
#include "labyrinth.h"

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


void player_move(player_t *player, char user_input, maze_t *maze);
void player_draw(player_t *player, uint8_t *framebuf);
bool player_collision(player_t *player, maze_t *maze);
grid_t player_grid_position(player_t *player, maze_t *maze);

#endif
