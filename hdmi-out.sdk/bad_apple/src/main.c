#include "display_ctrl/display_ctrl.h"
#include "xuartps.h"
#include "xil_cache.h"
#include "timer_ps/timer_ps.h"
#include "xparameters.h"
#include "xil_printf.h"
#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "qoi.h"
#include "video_frames.h"
#include "xscutimer.h"

#define printf xil_printf


/* Global variables */
DisplayCtrl disp_ctrl; // Display controller instance
XAxiVdma vdma; // Video DMA instance
u8 frame_buf[FRAME_SIZE] __attribute__((aligned(32))); // Single frame buffer

extern XScuTimer TimerInstance;

int
main(void)
{
    // Initialize the display system
    initialize_display();

	struct Frame* frame = &VideoFrames[0];

	// Decode QOI frame
	qoi_header_t header = qoi_decode_header(frame->data);
	//xil_printf("Width: %d, Height: %d\r\n", header.width, header.height);

	int buffer_size = header.width * header.height;

	RGBa_t *pixels = calloc(sizeof(RGBa_t), buffer_size);
	//memset(pixels, 0, buffer_size*sizeof(RGBa_t));

	qoi_decode(frame->data, frame->size, pixels, &header);

	// Clear the frame buffer
	//memset(frame_buf, 0, FRAME_SIZE);
	for (int y = 0; y < DISPLAY_HEIGHT; y++) {
		for (int x = 0; x < DISPLAY_WIDTH; x++) {
			FRAME_PIXEL_B(frame_buf, x, y) = 0x00;
			FRAME_PIXEL_G(frame_buf, x, y) = 0x00;
			FRAME_PIXEL_R(frame_buf, x, y) = 0xFF;
			FRAME_PIXEL_A(frame_buf, x, y) = 0xFF;
		}
	}
	Xil_DCacheFlushRange((unsigned int) frame_buf, FRAME_SIZE);

//	for (int i = 0; i < FRAME_SIZE; i++) {
//		//printf("%06d ", i);
//		frame_buf[i] = 0x00;
//		u8 fbuff_i = frame_buf[i];
//	}
//	memset(frame_buf, 0, FRAME_SIZE);

	// Calculate the top-left corner position to center the frame
	//int start_x = (DISPLAY_WIDTH - header.width) / 2;
	//int start_y = (DISPLAY_HEIGHT - header.height) / 2;
	int start_x = 0;
	int start_y = 0;

	int frame_num = 0;
    const u32 target_frame_time = 1000000/30; // Target 30fps in microseconds

	// Main loop - continuously update the display
	while (1) {
		XScuTimer_Stop(&TimerInstance);
		XScuTimer_DisableAutoReload(&TimerInstance);
		XScuTimer_LoadTimer(&TimerInstance, TIMER_FREQ_HZ/1000); // Load with 1ms worth of ticks
		XScuTimer_Start(&TimerInstance);
		u32 start_time = XScuTimer_GetCounterValue(&TimerInstance);

		frame = &VideoFrames[frame_num];
		qoi_decode(frame->data, frame->size, pixels, &header);

		// Copy decoded pixels to frame buffer
		for (int y = 0; y < header.height; y++) {
		    for (int x = 0; x < header.width; x++) {
		        RGBa_t pixel = pixels[y * header.width + x];

		        // Calculate base position for the 4x4 block
		        int base_x = start_x + (x * 4);
		        int base_y = start_y + (y * 4);

		        // Draw 4x4 block of pixels
		        for (int dy = 0; dy < 4; dy++) {
		            for (int dx = 0; dx < 4; dx++) {
		                int offset_x = base_x + dx;
		                int offset_y = base_y + dy;

		                // Check if we're still within display bounds
		                if (offset_x < DISPLAY_WIDTH && offset_y < DISPLAY_HEIGHT) {
		                    FRAME_PIXEL_R(frame_buf, offset_x, offset_y) = pixel.red;
		                    FRAME_PIXEL_G(frame_buf, offset_x, offset_y) = pixel.green;
		                    FRAME_PIXEL_B(frame_buf, offset_x, offset_y) = pixel.blue;
		                    FRAME_PIXEL_A(frame_buf, offset_x, offset_y) = 255;
		                }
		            }
		        }
		    }
		}

		frame_num += 1;
		frame_num = (frame_num <= 6571) ? frame_num : 0;

		// Flush cache to ensure the frame buffer is written to memory
		Xil_DCacheFlushRange((unsigned int) frame_buf, FRAME_SIZE);

		u32 end_time = XScuTimer_GetCounterValue(&TimerInstance);
		u32 elapsed_us = ((start_time - end_time) * 1000) / (TIMER_FREQ_HZ / 1000); // Convert to microseconds

		printf("End time: %d\r\n", end_time);
		printf("Elapsed_us: %d\r\n", elapsed_us);
		printf("Timer delay: %d\r\n", target_frame_time - elapsed_us);
		printf("\r\n");

		if (elapsed_us < target_frame_time) {
			TimerDelay(target_frame_time - elapsed_us);
		}
	}

	free(pixels);
}

void
initialize_display(void)
{
    int status;
    XAxiVdma_Config *vdma_config;

    memset(frame_buf, 0, FRAME_SIZE);

    // Initialize timer for delays
    TimerInitialize(SCU_TIMER_ID);

    // Initialize VDMA driver
    vdma_config = XAxiVdma_LookupConfig(VGA_VDMA_ID);
    if (!vdma_config) {
        xil_printf("Error: Failed to find VDMA configuration\r\n");
        return;
    }

    status = XAxiVdma_CfgInitialize(&vdma, vdma_config, vdma_config->BaseAddress);
    if (status != XST_SUCCESS) {
        xil_printf("Error: VDMA initialization failed\r\n");
        return;
    }

    // Initialize display controller with single frame buffer
    status = DisplayInitialize(&disp_ctrl, &vdma, DISP_VTC_ID, DYNCLK_BASEADDR, frame_buf, DISPLAY_STRIDE);
    if (status != XST_SUCCESS) {
        xil_printf("Error: Display controller initialization failed\r\n");
        return;
    }

    // Set 640x480 resolution and start the display
    DisplaySetMode(&disp_ctrl, &VMODE_640x480);
    status = DisplayStart(&disp_ctrl);
    if (status != XST_SUCCESS) {
        xil_printf("Error: Failed to start display\r\n");
        return;
    }

	Xil_DCacheFlushRange((unsigned int) frame_buf, FRAME_SIZE);
}

void
draw_frame(void)
{
}
