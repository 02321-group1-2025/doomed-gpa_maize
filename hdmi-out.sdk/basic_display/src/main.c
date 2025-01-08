#include "display_ctrl/display_ctrl.h"
#include "xuartps.h"
#include "xil_cache.h"
#include "timer_ps/timer_ps.h"
#include "xparameters.h"
#include "xil_printf.h"
#include <string.h>
#include <stdlib.h>

#include "main.h"
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


    const u32 target_frame_time = 1000000/60;

    int x = 60;
    int y = 60;

    int vel_x = 4;
    int vel_y = 4;

    int width = 8;
    int height = 8;

	// Main loop - continuously update the display
	for (;;) {
		XScuTimer_Stop(&TimerInstance);
		XScuTimer_DisableAutoReload(&TimerInstance);
		XScuTimer_LoadTimer(&TimerInstance, TIMER_FREQ_HZ/1000); // Load with 1ms worth of ticks
		XScuTimer_Start(&TimerInstance);
		u32 start_time = XScuTimer_GetCounterValue(&TimerInstance);

		for (int y = 0; y < DISPLAY_HEIGHT; y++) {
			for (int x = 0; x < DISPLAY_WIDTH; x++) {
				PIXEL(frame_buf,0xFF,0,0,x,y);
			}
		}


		rect(frame_buf, 0xFF, 0xFF, 0xFF, x, y, width, height, 1);

		if (x > (DISPLAY_WIDTH-1) - width || x <= 0) {
			vel_x = -vel_x;
		}

		if (y > (DISPLAY_HEIGHT-1) - (height) || y <= 0) {
			vel_y = -vel_y;
		}

		x += vel_x;
		y += vel_y;



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

void line(u8* frame, u8 red, u8 green, u8 blue, int x0, int y0, int x1, int y1){

	if(x0 == x1){ //vertical linje
		if(y0 > y1){
				int temp = y0;
				y0 = y1;
				y1 = temp;
		}
		for (int y = y0; y <= y1; y++){
			PIXEL(frame,red,green,blue,x0,y);
		}
	}else if(y0 == y1){//horizontal linje
		if(y0 > y1){
			int temp = y0;
			y0 = y1;
			y1 = temp;
		}
		for (int x = x0; x <= x1; x++){
			PIXEL(frame,red,green,blue,x,y0);
		}
	}else if(abs(y1-y0) < abs(x1 -x0)){//low

		if(x0 > x1){
			int temp = x0;
			x0 = x1;
			x1 = temp;
			temp = y0;
			y0 = y1;
			y1 = temp;
		}

		int dx = x1 - x0;
		int dy = y1 - y0;
		int yi = 1;
		if (dy < 0){
			yi = -1;
			dy = -dy;
		}
		int D = (2 * dy) - dx;
		int y = y0;

		for (int x = x0; x <= x1; x++){
			PIXEL(frame,red,green,blue,x,y);
			if (D > 0){
				y += yi;
				D += (2 * (dy - dx));
			}else{
				D += 2*dy;
			}
		}
	}else{//high

		if(y0 > y1){
			int temp = x0;
			x0 = x1;
			x1 = temp;
			temp = y0;
			y0 = y1;
			y1 = temp;
		}

		int dx = x1 - x0;
		int dy = y1 - y0;
		int xi = 1;
		if (dx < 0){
			xi = -1;
			dx = -dx;
		}
		int D = (2 * dx) - dy;
		int x = x0;

		for (int y = y0; y <= y1;y++){
			PIXEL(frame,red,green,blue,x,y);
			if (D > 0){
				x += xi;
				D += (2 * (dx - dy));
			}else{
				D += 2*dx;
			}
		}
	}
}

void rect(u8* frame, u8 red, u8 green, u8 blue, int x0, int y0, int width, int height, u8 fill){
	if (fill) {
	    for (int x = x0; x <= x0 + width; x++) {
	        for (int y = y0; y <= y0 + height; y++) {
	            PIXEL(frame,red,green,blue,x,y);
	        }
	    }
	} else {
	    for (int x = x0; x <= x0 + width; x++) {

	    	PIXEL(frame, red, green, blue, x, y0         );//top
			PIXEL(frame, red, green, blue, x, y0 + height);//bottom
	    }
	    for (int y = y0; y <= y0 + height; y++) {
	        PIXEL(frame, red, green, blue, x0        , y );//left
	        PIXEL(frame, red, green, blue, x0 + width, y );//right
	    }
	}
}

void rectCenter(u8* frame, u8 red, u8 green, u8 blue, int x0, int y0, int width, int height, u8 fill){
	rect(frame,red,green,blue,x0-(width/2),y0-(height/2),width,height,fill);
}
