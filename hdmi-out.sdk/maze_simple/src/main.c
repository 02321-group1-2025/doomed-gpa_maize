#include "display_ctrl/display_ctrl.h"
#include "xuartps.h"
#include "xil_cache.h"
#include "timer_ps/timer_ps.h"
#include "xparameters.h"
#include "xil_printf.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "xgpio.h"

#include "main.h"
#include "xscutimer.h"
#include "draw.h"
#include "player.h"
#include "maze.h"
#include "solver.h"
#include "display.h"
#include "buttons.h"
#include "qoi.h"
#include "video_frames.h"

#include "labyrinth.h"

#define printf xil_printf

#define NUM_FRAMES 2

void ray_casting(u8 *framebuf, player_t *player, maze_t *maze);
uint16_t* showHint(maze_t* maze, point_t* hint, uint8_t* delay);
point_t* generateHint(maze_t* maze, uint16_t playerX, uint16_t playerY, uint16_t endX, uint16_t endY, uint8_t quick);
void resetHint(maze_t* maze, uint8_t* delay, uint16_t* old);


/* Global variables */
DisplayCtrl disp_ctrl; // Display controller instance
XAxiVdma vdma; // Video DMA instance

u8 *frame_ptr[NUM_FRAMES];
u8 maze_buf[FRAME_SIZE] __attribute__((aligned(32)));
u8 frame_buf[NUM_FRAMES][FRAME_SIZE] __attribute__((aligned(32))); // Single frame buffer
const int FPS = 20;

//const float FOV = (90 * 2 * M_PI)/360;
const float FOV = 0.85f;
const float fisheye_correction = 1.0f;

//const int WALL_SCALE = DISPLAY_HEIGHT / 32;
const int WALL_SCALE = 30;

//const float STEP_SIZE = 1.0f;
const float STEP_SIZE = 1.0f;

extern XScuTimer TimerInstance;

int
main(void)
{
    // Initialize the display system
    initialize_display();

	int next_frame = 0;
	//u8 *frame = disp_ctrl.framePtr[disp_ctrl.curFrame];

	// Clear the frame buffer
	//memset(frame_buf, 0, FRAME_SIZE);
	for (int y = 0; y < DISPLAY_HEIGHT; y++) {
		for (int x = 0; x < DISPLAY_WIDTH; x++) {
			FRAME_PIXEL_B(frame_buf[0], x, y) = 0x00;
			FRAME_PIXEL_G(frame_buf[0], x, y) = 0x00;
			FRAME_PIXEL_R(frame_buf[0], x, y) = 0xFF;
			FRAME_PIXEL_A(frame_buf[0], x, y) = 0xFF;
		}
	}
	Xil_DCacheFlushRange((unsigned int) frame_buf[0], FRAME_SIZE);

	for (int y = 0; y < DISPLAY_HEIGHT; y++) {
		for (int x = 0; x < DISPLAY_WIDTH; x++) {
			FRAME_PIXEL_B(frame_buf[1], x, y) = 0xFF;
			FRAME_PIXEL_G(frame_buf[1], x, y) = 0x00;
			FRAME_PIXEL_R(frame_buf[1], x, y) = 0x00;
			FRAME_PIXEL_A(frame_buf[1], x, y) = 0xFF;
		}
	}
	Xil_DCacheFlushRange((unsigned int) frame_buf[1], FRAME_SIZE);

	/* Flush UART FIFO */
	while (XUartPs_IsReceiveData(UART_BASEADDR)) {
		XUartPs_ReadReg(UART_BASEADDR, XUARTPS_FIFO_OFFSET);
	}

	char user_input = 0;

    const u32 target_frame_time = 1000000 / FPS; // Target 30fps in microseconds
    const u32 timer_count = (TIMER_FREQ_HZ / 1000000) * target_frame_time;

    player_t player = {
    		.x = 16,
			.y = 12,
			.angle = M_PI/2,

			.collision = true,

			.size = 10,
    };

    button_state_t button_state = {0};
    initialize_buttons(&button_state);
    button_setup_interrupts(&button_state);

	uint32_t rng_seed = 0;

    uint16_t end_x = 19, end_y = 19;
    uint32_t frameCounter = 0;
    uint32_t total_time = 0; // Update when player finish
    uint32_t total_completions = 0;
    uint32_t lap_time = 0;
    maze_t *maze = NULL;
    uint8_t result = 0;
    uint16_t color = makeColor(0xFF,0x00,0x00);


    const struct Frame* frame = &VideoFrames[0];

    // Decode QOI frame
	qoi_header_t header = qoi_decode_header(frame->data);
	RGBa_t *pixels = calloc(sizeof(RGBa_t), header.width * header.height);

	qoi_decode(frame->data, frame->size, pixels, &header);
reset:
	// Copy decoded pixels to frame buffer
	for (int y = 0; y < header.height; y++) {
		for (int x = 0; x < header.width; x++) {
			RGBa_t pixel = pixels[y * header.width + x];

			// Calculate base position for the 4x4 block
			int base_x = (x * 4);
			int base_y = (y * 4);

			// Draw 4x4 block of pixels
			for (int dy = 0; dy < 4; dy++) {
				for (int dx = 0; dx < 4; dx++) {
					int offset_x = base_x + dx;
					int offset_y = base_y + dy;

					// Check if we're still within display bounds
					if (offset_x < DISPLAY_WIDTH && offset_y < DISPLAY_HEIGHT) {
						FRAME_PIXEL_R(disp_ctrl.framePtr[disp_ctrl.curFrame], offset_x, offset_y) = pixel.red;
						FRAME_PIXEL_G(disp_ctrl.framePtr[disp_ctrl.curFrame], offset_x, offset_y) = pixel.green;
						FRAME_PIXEL_B(disp_ctrl.framePtr[disp_ctrl.curFrame], offset_x, offset_y) = pixel.blue;
						FRAME_PIXEL_A(disp_ctrl.framePtr[disp_ctrl.curFrame], offset_x, offset_y) = 255;
					}
				}
			}
		}
	}
	Xil_DCacheFlushRange((unsigned int) disp_ctrl.framePtr[disp_ctrl.curFrame], FRAME_SIZE);

	while (1) {
		user_input = 0;
		if (XUartPs_IsReceiveData(UART_BASEADDR)){
			user_input = XUartPs_ReadReg(UART_BASEADDR, XUARTPS_FIFO_OFFSET);
		}
		printf("%d\r\n", button_state.btn_val);

		if (user_input != 0) break;
		if (button_state.btn_val != 0) break;

		rng_seed += 1;
	}

    do{
    	rng_seed += 1;
    	srand(rng_seed);
    	if(maze != NULL){
    		free(maze->data);
    		free(maze);
    	}
    	maze = BuildMaze (20, 20, 0, 100, 0, 0);
    	result = labyrinthTest(maze,end_x,end_y);
    }while(result != 0 && result != 0b100);

    for (int i = 0; i < maze->width; i++) {
    	for (int j = 0; j < maze->height; j++) {
    		setColor(maze, i, j, makeColor(0xFF, 0xFF, 0xFF));
    	}
    }

    // Set end square to red

    setColor(maze, end_x, end_y, color);

    // Set end to a path
    setPath(maze, end_x, end_y, 0);

    generate_maze_buffer(maze, maze_buf);

	bool map_toggle = false;

	// Main loop - continuously update the display
	while (1) {
		XScuTimer_Stop(&TimerInstance);
		XScuTimer_DisableAutoReload(&TimerInstance);
		XScuTimer_LoadTimer(&TimerInstance, timer_count);  // Load maximum value
		XScuTimer_Start(&TimerInstance);
		u32 start_time = XScuTimer_GetCounterValue(&TimerInstance);

		next_frame = disp_ctrl.curFrame + 1;
		if (next_frame >= NUM_FRAMES) {
			next_frame = 0;
		}

		u8 *cur_frame_ptr = disp_ctrl.framePtr[next_frame];

		memset(cur_frame_ptr, 0, FRAME_SIZE);

		user_input = 0;
		if (XUartPs_IsReceiveData(UART_BASEADDR)){
			user_input = XUartPs_ReadReg(UART_BASEADDR, XUARTPS_FIFO_OFFSET);
		}

		if (user_input == 'm' || user_input == 'M' || button_state.btn_val == BUTTON_HINT) {
			map_toggle = !map_toggle;
		}

		if (user_input == 'c') {
			player.collision = !player.collision;
		}

		if (user_input == 'r' || button_state.btn_val == BUTTON_ESC){
			free(maze->data);
			free(maze);
			end_x = 19, end_y = 19;
			frameCounter = 0;
			total_time = 0; // Update when player finish
			total_completions = 0;
			lap_time = 0;
			maze = NULL;
			result = 0;
			color = makeColor(0xFF,0x00,0x00);
			player.x = 16;
			player.y = 12;
			goto reset;
		}

		// Clear frame pointer
		memset(cur_frame_ptr, 0, FRAME_SIZE);

		if (map_toggle) {
			// Draw maze
			memcpy(cur_frame_ptr, maze_buf, FRAME_SIZE);

			// This can crash the program. w/e
			player_draw(&player, cur_frame_ptr);
		}
		else {
			// "Skybox"
			rect(cur_frame_ptr,
				 0x87, 0xCE, 0xEB, // Sky blue
				 0, 0,
				 DISPLAY_WIDTH, (DISPLAY_HEIGHT/2),
				 1
				 );

			// "Ground"
			rect(cur_frame_ptr,
				 0x32, 0xCD, 0x32, // Lime green
				 0, (DISPLAY_HEIGHT/2),
				 DISPLAY_WIDTH, (DISPLAY_HEIGHT/2),
				 1
				 );

			ray_casting(cur_frame_ptr, &player, maze);
		}

		if (map_toggle == false) { // Only move the player if the map is in 3D
			player_move(&player, user_input, button_state.btn_val, maze,end_x,end_y);
		}


		frameCounter++;
		//in case of winning (unlikely)
		grid_t position = player_grid_position(&player, maze);
		if(position.col == end_x && position.row == end_y){
			total_completions += 1;
			total_time += frameCounter;
			lap_time = frameCounter;

			if(end_x != 0){
				end_x = 0;
				end_y = 0;
			}else{
				end_x = 19;
				end_y = 19;
			}
			rng_seed += frameCounter;
			frameCounter = 0;
			do{
				rng_seed += 1;
				srand(rng_seed);
				if(maze != NULL){
					free(maze->data);
					free(maze);
				}
				maze =  BuildMaze (20, 20, 0, 100, rand() % 20, rand() % 20);
				result = labyrinthTest(maze,end_x,end_y);
				result |= labyrinthTest(maze,position.col ,position.row);
			}while(result != 0 && result != 0b100);
			for (int i = 0; i < maze->width; i++) {
			       	for (int j = 0; j < maze->height; j++) {
			       		setColor(maze, i, j, makeColor(0xFF, 0xFF, 0xFF));
			       	}
			    }

			    // Set end square to red
			    uint16_t color = makeColor(0xFF,0x00,0x00);
			    setColor(maze, end_x, end_y, color);

			    // Set end to a path
			    setPath(maze, end_x, end_y, 0);

			    generate_maze_buffer(maze, maze_buf);
		}

		// Flush cache to ensure the frame buffer is written to memory
		Xil_DCacheFlushRange((unsigned int) cur_frame_ptr, FRAME_SIZE);

		u32 end_time = XScuTimer_GetCounterValue(&TimerInstance);
		u32 elapsed_us = ((timer_count - end_time) * 1000) / (TIMER_FREQ_HZ / 1000); // Convert to microseconds

		DisplayChangeFrame(&disp_ctrl, next_frame);

		u32 delay_us = 0;
		u32 delay_ticks = 0;

		if (elapsed_us < target_frame_time) {
		    delay_us = target_frame_time - elapsed_us;

		    // Convert microseconds to timer ticks
		    delay_ticks = (delay_us * (TIMER_FREQ_HZ / 1000)) / 1000;



		    //while (XScuTimer_GetCounterValue(&TimerInstance)) {}
		}
		while (XScuTimer_GetCounterValue(&TimerInstance)) {}

		printf("\033[2J"); // Clear screen
		printf("\033[H");  // Move cursor to home
		printf("Start time: %d\r\n", start_time);
		printf("End time: %d\r\n", end_time);
		printf("Elapsed_us: %d\r\n", elapsed_us);
		printf("Timer delay: %d\r\n\n", delay_ticks);

		//grid_t grid = player_grid_position(&player);
		printf("Player grid: (%d, %d)\r\n", player.grid_pos.col, player.grid_pos.row);
		printf("User input: %c\r\n", user_input);
		printf("Player angle: %f\r\n", player.angle);

		printf("Lap time: %ds\r\n", (int)lap_time / FPS);
		printf("Completion time: %ds\r\n", (int)total_time / FPS);
		printf("Total maze completions: %d\r\n\n", total_completions);

		printf("FOV: %d.%03d\r\n", (int)FOV, (int)((FOV - (int)FOV) * 1000));
		printf("Fisheye correction: %d.%03d\r\n", (int)fisheye_correction, (int)((fisheye_correction - (int)fisheye_correction) * 1000));

		printf("Wall scale: %d\r\n", WALL_SCALE);
		printf("Step_size: %d.%03d\r\n", (int)STEP_SIZE, (int)((STEP_SIZE - (int)STEP_SIZE) * 1000));
	}
}

void
initialize_display(void) {
	int Status;
	XAxiVdma_Config *vdmaConfig;
	int i;

	/*
	 * Initialize an array of pointers to the 3 frame buffers
	 */
	for (i = 0; i < DISPLAY_NUM_FRAMES; i++)
	{
		frame_ptr[i] = frame_buf[i];
	}

	/*
	 * Initialize a timer used for a simple delay
	 */
	TimerInitialize(SCU_TIMER_ID);

	/*
	 * Initialize VDMA driver
	 */
	vdmaConfig = XAxiVdma_LookupConfig(VGA_VDMA_ID);
	if (!vdmaConfig)
	{
		xil_printf("No video DMA found for ID %d\r\n", VGA_VDMA_ID);
		return;
	}
	Status = XAxiVdma_CfgInitialize(&vdma, vdmaConfig, vdmaConfig->BaseAddress);
	if (Status != XST_SUCCESS)
	{
		xil_printf("VDMA Configuration Initialization failed %d\r\n", Status);
		return;
	}

	/*
	 * Initialize the Display controller and start it
	 */

	Status = DisplayInitialize(&disp_ctrl, &vdma, DISP_VTC_ID, DYNCLK_BASEADDR, frame_ptr, DISPLAY_STRIDE);
	if (Status != XST_SUCCESS)
	{
		xil_printf("Display Ctrl initialization failed during demo initialization%d\r\n", Status);
		return;
	}
	Status = DisplayStart(&disp_ctrl);
	if (Status != XST_SUCCESS)
	{
		xil_printf("Couldn't start display during demo initialization%d\r\n", Status);
		return;
	}
}

void
draw_frame(void)
{
}

void
draw_maze(uint8_t *maze, uint8_t *frame_buf, uint8_t *maze_buf) {
    const int CELL_WIDTH = DISPLAY_WIDTH / MAZE_SIZE;  // 32 pixels
    const int CELL_HEIGHT = DISPLAY_HEIGHT / MAZE_SIZE; // 24 pixels

    // First draw to maze buffer
    for (int row = 0; row < MAZE_SIZE; row++) {
        for (int col = 0; col < MAZE_SIZE; col++) {
            // Calculate the pixel coordinates for this cell
            int x = col * CELL_WIDTH;
            int y = row * CELL_HEIGHT;

            if (maze[row * MAZE_SIZE + col]) {
                // Draw wall in white
                rect(maze_buf, 0xFF, 0xFF, 0xFF, x, y, CELL_WIDTH, CELL_HEIGHT, 1);
            } else {
                // Draw path in dark gray
                rect(maze_buf, 0x20, 0x20, 0x20, x, y, CELL_WIDTH, CELL_HEIGHT, 1);
            }
        }
    }

    // Copy maze buffer to frame buffer
    memcpy(frame_buf, maze_buf, FRAME_SIZE);
}

void
generate_maze_buffer(maze_t *maze, uint8_t *maze_buf) {
    const int CELL_WIDTH = DISPLAY_WIDTH / maze->width;  // 32 pixels
    const int CELL_HEIGHT = DISPLAY_HEIGHT / maze->height; // 24 pixels

    // Clear buffer first
    memset(maze_buf, 0, FRAME_SIZE);

    // Generate the maze visualization once
    for (int row = 0; row < maze->height; row++) {
        for (int col = 0; col < maze->width; col++) {
            int x = col * CELL_WIDTH;
            int y = row * CELL_HEIGHT;

            uint16_t grid_color = getColor(maze, col, row);
            uint8_t grid_r = (getRed(grid_color)*255)/31;
            uint8_t grid_g = (getGreen(grid_color)*255)/31;
            uint8_t grid_b = (getBlue(grid_color)*255)/31;


            int path_debug = isPath(maze, col, row);

            if (!path_debug) {
                // Draw wall
                rect(maze_buf, grid_r, grid_g, grid_b, x, y, CELL_WIDTH, CELL_HEIGHT, 1);
            } else {
                // Draw path in dark gray
                rect(maze_buf, 0x20, 0x20, 0x20, x, y, CELL_WIDTH, CELL_HEIGHT, 1);
            }
        }
    }

    // Ensure maze buffer is written to memory
    Xil_DCacheFlushRange((unsigned int) maze_buf, FRAME_SIZE);
}

void
ray_casting(u8 *framebuf, player_t *player, maze_t *maze) {

    const int NUM_RAYS = DISPLAY_WIDTH;  // One ray per vertical screen column
    const float ANGLE_STEP = FOV / NUM_RAYS;  // Angle between rays

    float start_angle = player->angle - FOV/2;


    const int MAX_DEPTH = (maze->width > maze->height ? maze->width : maze->height) * (DISPLAY_WIDTH / maze->width); // Maximum ray length


    // Cast rays across the FOV
	for(int i = 0; i < NUM_RAYS; i++) {
		float ray_angle = start_angle + (i * ANGLE_STEP);

		// Normalize angle
		while(ray_angle < 0) ray_angle += 2*M_PI;
		while(ray_angle >= 2*M_PI) ray_angle -= 2*M_PI;

		float ray_x = player->x;
		float ray_y = player->y;
		float x_step = cosf(ray_angle) * STEP_SIZE;
		float y_step = sinf(ray_angle) * STEP_SIZE;

		int map_x = ray_x / (DISPLAY_WIDTH / maze->width);
		int map_y = ray_y / (DISPLAY_HEIGHT / maze->height);

		int prev_map_x = ray_x / (DISPLAY_WIDTH / maze->width);
		int prev_map_y = ray_y / (DISPLAY_HEIGHT / maze->height);
		bool is_vertical_edge = false;
		bool is_horizontal_edge = false;

		int dof = 0;
		while(dof < MAX_DEPTH) {
			map_x = ray_x / (DISPLAY_WIDTH / maze->width);
			map_y = ray_y / (DISPLAY_HEIGHT / maze->height);

            // Detect cell transitions
            if (map_x != prev_map_x) {
                // Check if this vertical edge is between wall and space
                if (!isPath(maze, map_x, map_y) != !isPath(maze, prev_map_x, map_y)) {
                    is_vertical_edge = true;
                    break;
                }
            }
            if (map_y != prev_map_y) {
                // Check if this horizontal edge is between wall and space
                if (!isPath(maze, map_x, map_y) != !isPath(maze, map_x, prev_map_y)) {
                    is_horizontal_edge = true;
                    break;
                }
            }


			// Check bounds and wall collision
			if(!isPath(maze,(int)map_x,(int)map_y)) {
				break;
			}

			prev_map_x = map_x;
			prev_map_y = map_y;
			ray_x += x_step;
			ray_y += y_step;
			dof += STEP_SIZE;
		}

		// Calculate wall height based on distance
		float distance = sqrtf((ray_x - player->x) * (ray_x - player->x) +
							   (ray_y - player->y) * (ray_y - player->y));

		// Apply fisheye correction
		float corrected_distance = fisheye_correction * distance * cosf(ray_angle - player->angle);

		// Calculate wall height (inversely proportional to distance)
		int wall_height = (DISPLAY_HEIGHT / corrected_distance) * WALL_SCALE;

		// Calculate vertical wall strip height
		int wall_top = (DISPLAY_HEIGHT - wall_height) / 2;
		int wall_bottom = wall_top + wall_height;

		// Make sure not to make a line that is out of bounds of frame buffer
		if (wall_top < 0) wall_top = 0;
		if (wall_bottom > DISPLAY_HEIGHT-1) wall_bottom = DISPLAY_HEIGHT-1;

		// Draw vertical line for this ray
		//uint8_t wall_r = 0xFF;
		//uint8_t wall_g = 0xFF;
		//uint8_t wall_b = 0xFF;

		uint16_t maze_wall_color = getColor(maze, map_x, map_y);
		uint8_t wall_r = (getRed(maze_wall_color) * 255) / 31;
		uint8_t wall_g = (getGreen(maze_wall_color) * 255) / 31;
		uint8_t wall_b = (getBlue(maze_wall_color)  * 255) / 31;

		if (is_vertical_edge) {
			// Vertical edges appear darker
			wall_r *= 0.7f;
			wall_g *= 0.7f;
			wall_b *= 0.7f;
		} else if (is_horizontal_edge) {
			// Horizontal edges appear lighter
			wall_r *= 0.9f;
			wall_g *= 0.9f;
			wall_b *= 0.9f;
		}

		// Add distance-based darkening
		float distance_factor = 1.0f - (distance / MAX_DEPTH);
		wall_r = (uint8_t)(wall_r * distance_factor);
		wall_g = (uint8_t)(wall_g * distance_factor);
		wall_b = (uint8_t)(wall_b * distance_factor);

		line(framebuf,
			 wall_r, wall_g, wall_b,
		     i, wall_top,                  // Start point (x, y)
		     i, wall_bottom);              // End point (x, same x, different y)

		POINT(framebuf,
			  wall_r*0.8f, wall_g*0.8f, wall_b*0.8f,
			  i, wall_top);
		POINT(framebuf,
			  wall_r*0.8f, wall_g*0.8f, wall_b*0.8f,
			  i, wall_bottom);
	}
}

point_t* generateHint(maze_t* maze, uint16_t playerX, uint16_t playerY, uint16_t endX, uint16_t endY, uint8_t quick){
	maze->startX = playerX;
	maze->startY = playerX;
	return solve(maze,endX,endY,quick);
}

uint16_t* showHint(maze_t* maze, point_t* hint, uint8_t* delay){
	uint16_t* old = maze->data;
	uint16_t* new = malloc(maze->width * maze->height * sizeof(uint16_t));
	if(new == NULL){
		return NULL;
	}
	memcpy(new,old,maze->width * maze->height * sizeof(uint16_t));
	*delay = 16;
	uint16_t color = makeColor(0xFF,0x06,0x06);
	point_t* temp = hint;
	while(temp != NULL){
		setColor(maze,temp->X+1,temp->Y  ,color);
		setColor(maze,temp->X-1,temp->Y  ,color);
		setColor(maze,temp->X  ,temp->Y+1,color);
		setColor(maze,temp->X  ,temp->Y-1,color);
		temp = temp->recursivePoint;
	}
	return new;
}
void resetHint(maze_t* maze, uint8_t* delay, uint16_t* old){
	if(delay == NULL || old == NULL) return;
	if(*delay == 0){
		free(maze->data);
		maze->data = old;
	}else{
		*delay = *delay-1;
	}
}
