/************************************************************************/
/*																		*/
/*	display_demo.c	--	ZYBO Display demonstration 						*/
/*																		*/
/************************************************************************/
/*	Author: Sam Bobrowicz												*/
/*	Copyright 2016, Digilent Inc.										*/
/************************************************************************/
/*  Module Description: 												*/
/*																		*/
/*		This file contains code for running a demonstration of the		*/
/*		HDMI output capabilities on the ZYBO. It is a good	            */
/*		example of how to properly use the display_ctrl drivers.	    */
/*																		*/
/************************************************************************/
/*  Revision History:													*/
/* 																		*/
/*		2/5/2016(SamB): Created											*/
/*																		*/
/************************************************************************/

/* ------------------------------------------------------------ */
/*				Include File Definitions						*/
/* ------------------------------------------------------------ */

#include "display_demo.h"
#include "display_ctrl/display_ctrl.h"
#include <stdio.h>
#include "xuartps.h"
#include "math.h"
#include <ctype.h>
#include <stdlib.h>
#include "xil_types.h"
#include "xil_cache.h"
#include "timer_ps/timer_ps.h"
#include "xparameters.h"

/*
 * XPAR redefines
 */
#define DYNCLK_BASEADDR XPAR_AXI_DYNCLK_0_BASEADDR
#define VGA_VDMA_ID XPAR_AXIVDMA_0_DEVICE_ID
#define DISP_VTC_ID XPAR_VTC_0_DEVICE_ID
#define VID_VTC_IRPT_ID XPS_FPGA3_INT_ID
#define VID_GPIO_IRPT_ID XPS_FPGA4_INT_ID
#define SCU_TIMER_ID XPAR_SCUTIMER_DEVICE_ID
#define UART_BASEADDR XPAR_PS7_UART_1_BASEADDR

/* ------------------------------------------------------------ */
/*				Global Variables								*/
/* ------------------------------------------------------------ */

/*
 * Display Driver structs
 */
DisplayCtrl dispCtrl;
XAxiVdma vdma;

/*
 * Framebuffers for video data
 */
u8  frameBuf[DISPLAY_NUM_FRAMES][DEMO_MAX_FRAME] __attribute__((aligned(0x20)));
u8 *pFrames[DISPLAY_NUM_FRAMES]; //array of pointers to the frame buffers

/* ------------------------------------------------------------ */
/*				Procedure Definitions							*/
/* ------------------------------------------------------------ */

int main(void)
{
	DemoInitialize();

	DemoRun();

	return 0;
}


void DemoInitialize()
{
	int Status;
	XAxiVdma_Config *vdmaConfig;
	int i;

	/*
	 * Initialize an array of pointers to the 3 frame buffers
	 */
	for (i = 0; i < DISPLAY_NUM_FRAMES; i++)
	{
		pFrames[i] = frameBuf[i];
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

	Status = DisplayInitialize(&dispCtrl, &vdma, DISP_VTC_ID, DYNCLK_BASEADDR, pFrames, DEMO_STRIDE);
	if (Status != XST_SUCCESS)
	{
		xil_printf("Display Ctrl initialization failed during demo initialization%d\r\n", Status);
		return;
	}
	Status = DisplayStart(&dispCtrl);
	if (Status != XST_SUCCESS)
	{
		xil_printf("Couldn't start display during demo initialization%d\r\n", Status);
		return;
	}

	DemoPrintTest(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, dispCtrl.stride, DEMO_PATTERN_1);

	return;
}

#define FRAME_PIXEL(x, y, stride) ((y) * (stride) + (x) * 4)
#define FRAME_PIXEL_B(frame, x, y, stride) frame[FRAME_PIXEL(x, y, stride)]
#define FRAME_PIXEL_G(frame, x, y, stride) frame[FRAME_PIXEL(x, y, stride) + 1]
#define FRAME_PIXEL_R(frame, x, y, stride) frame[FRAME_PIXEL(x, y, stride) + 2]
#define FRAME_PIXEL_A(frame, x, y, stride) frame[FRAME_PIXEL(x, y, stride) + 3]

void line(u8* frame, u8 red, u8 green, u8 blue, int x0, int y0, int x1, int y1){

	if(x0 == x1){ //vertical linje
		if(y0 > y1){
				int temp = y0;
				y0 = y1;
				y1 = temp;
		}
		for (int y = y0; y <= y1; y++){
			FRAME_PIXEL_R(frame,x0,y,dispCtrl.stride) = red;
			FRAME_PIXEL_G(frame,x0,y,dispCtrl.stride) = green;
			FRAME_PIXEL_B(frame,x0,y,dispCtrl.stride) = blue;
		}
	}else if(y0 == y1){//horizontal linje
		if(y0 > y1){
			int temp = y0;
			y0 = y1;
			y1 = temp;
		}
		for (int x = x0; x <= x1; x++){
			FRAME_PIXEL_R(frame,x,y0,dispCtrl.stride) = red;
			FRAME_PIXEL_G(frame,x,y0,dispCtrl.stride) = green;
			FRAME_PIXEL_B(frame,x,y0,dispCtrl.stride) = blue;
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
			FRAME_PIXEL_R(frame,x,y,dispCtrl.stride) = red;
			FRAME_PIXEL_G(frame,x,y,dispCtrl.stride) = green;
			FRAME_PIXEL_B(frame,x,y,dispCtrl.stride) = blue;
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
			FRAME_PIXEL_R(frame,x,y,dispCtrl.stride) = red;
			FRAME_PIXEL_G(frame,x,y,dispCtrl.stride) = green;
			FRAME_PIXEL_B(frame,x,y,dispCtrl.stride) = blue;
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
	if (fill){
		for(int x = x0; x <= x0 + width; x++){
			for (int y = y0; y < y0 + height; y++){
				FRAME_PIXEL_R(frame,x,y,dispCtrl.stride) = red;
				FRAME_PIXEL_G(frame,x,y,dispCtrl.stride) = green;
				FRAME_PIXEL_B(frame,x,y,dispCtrl.stride) = blue;
			}
		}
	}else{
		for(int x = x0; x <= x0 + width; x++){
			FRAME_PIXEL_R(frame,x,y0         ,dispCtrl.stride) = red;//top
			FRAME_PIXEL_G(frame,x,y0         ,dispCtrl.stride) = green;
			FRAME_PIXEL_B(frame,x,y0         ,dispCtrl.stride) = blue;

			FRAME_PIXEL_R(frame,x,y0 + height,dispCtrl.stride) = red;//bottom
			FRAME_PIXEL_G(frame,x,y0 + height,dispCtrl.stride) = green;
			FRAME_PIXEL_B(frame,x,y0 + height,dispCtrl.stride) = blue;
		}
		for(int y = y0; y <= y0 + width; y++){
			FRAME_PIXEL_R(frame,x0,y        ,dispCtrl.stride) = red;//left
			FRAME_PIXEL_G(frame,x0,y        ,dispCtrl.stride) = green;
			FRAME_PIXEL_B(frame,x0,y        ,dispCtrl.stride) = blue;

			FRAME_PIXEL_R(frame,x0 + width,y,dispCtrl.stride) = red; //right
			FRAME_PIXEL_G(frame,x0 + width,y,dispCtrl.stride) = green;
			FRAME_PIXEL_B(frame,x0 + width,y,dispCtrl.stride) = blue;
		}
	}
}

void DemoRun()
{

	/* Flush UART FIFO */
	while (XUartPs_IsReceiveData(UART_BASEADDR))
	{
		XUartPs_ReadReg(UART_BASEADDR, XUARTPS_FIFO_OFFSET);
	}

	u8 *frame = dispCtrl.framePtr[dispCtrl.curFrame];

	// Clear display every loop
			for (int y = 0; y < dispCtrl.vMode.height; y++) {
				for (int x = 0; x < dispCtrl.vMode.width; x++) {
					FRAME_PIXEL_R(frame, x, y, dispCtrl.stride) = 0x00;
					FRAME_PIXEL_G(frame, x, y, dispCtrl.stride) = 0x00;
					FRAME_PIXEL_B(frame, x, y, dispCtrl.stride) = 0x00;
				}
			}

	for (;;) {

		//   frame,r  ,g  ,b  ,x0 ,y0 ,x1 ,y1
		line(frame,255,000,000,000,000,639,000);//top
		line(frame,000,255,000,639,000,639,479);//rigth
		line(frame,000,000,255,000,479,639,479);//bottom
		line(frame,255,255,000,000,000,000,479);//left
		line(frame,255,000,255,000,000,639,479);//diag down
		line(frame,000,255,255,000,479,639,000);//diag up

		rect(frame,255,255,255,000,000,200,200, 0);//non solid
		rect(frame,255,255,255,062,062,100,100, 0);//solid
		Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);

		TimerDelay(10000);
	}

//	while (userInput != 'q')
//	{
//		DemoPrintMenu();
//
//		/* Wait for data on UART */
//		while (!XUartPs_IsReceiveData(UART_BASEADDR))
//		{}
//
//		/* Store the first character in the UART receive FIFO and echo it */
//		if (XUartPs_IsReceiveData(UART_BASEADDR))
//		{
//			userInput = XUartPs_ReadReg(UART_BASEADDR, XUARTPS_FIFO_OFFSET);
//			xil_printf("%c", userInput);
//		}
//
//		switch (userInput)
//		{
//		case '1':
//			DemoChangeRes();
//			break;
//		case '2':
//			nextFrame = dispCtrl.curFrame + 1;
//			if (nextFrame >= DISPLAY_NUM_FRAMES)
//			{
//				nextFrame = 0;
//			}
//			DisplayChangeFrame(&dispCtrl, nextFrame);
//			break;
//		case '3':
//			DemoPrintTest(pFrames[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, DEMO_STRIDE, DEMO_PATTERN_0);
//			break;
//		case '4':
//			DemoPrintTest(pFrames[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, DEMO_STRIDE, DEMO_PATTERN_1);
//			break;
//		case '5':
//		    DemoPrintTest(pFrames[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, DEMO_STRIDE, DEMO_PATTERN_2);
//		    break;
//		case '6': // Update existing case numbers
//		    DemoInvertFrame(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, dispCtrl.stride);
//		    break;
//		case '7':
//		    nextFrame = dispCtrl.curFrame + 1;
//		    // ... rest of existing code
//			nextFrame = dispCtrl.curFrame + 1;
//			if (nextFrame >= DISPLAY_NUM_FRAMES)
//			{
//				nextFrame = 0;
//			}
//			DemoInvertFrame(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.framePtr[nextFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, dispCtrl.stride);
//			DisplayChangeFrame(&dispCtrl, nextFrame);
//			break;
//		case 'q':
//			break;
//		default :
//			xil_printf("\n\rInvalid Selection");
//			TimerDelay(500000);
//		}
//	}

	return;
}

void DemoPrintMenu()
{
	xil_printf("\x1B[H"); //Set cursor to top left of terminal
	xil_printf("\x1B[2J"); //Clear terminal
	xil_printf("**************************************************\n\r");
	xil_printf("*               ZYBO Display Demo                *\n\r");
	xil_printf("**************************************************\n\r");
	xil_printf("*Display Resolution: %28s*\n\r", dispCtrl.vMode.label);
	printf("*Display Pixel Clock Freq. (MHz): %15.3f*\n\r", dispCtrl.pxlFreq);
	xil_printf("*Display Frame Index: %27d*\n\r", dispCtrl.curFrame);
	xil_printf("**************************************************\n\r");
	xil_printf("\n\r");
	xil_printf("1 - Change Display Resolution\n\r");
	xil_printf("2 - Change Display Framebuffer Index\n\r");
	xil_printf("3 - Print Blended Test Pattern to Display Framebuffer\n\r");
	xil_printf("4 - Print Color Bar Test Pattern to Display Framebuffer\n\r");
	xil_printf("5 - Print Dual Color Bar Test Pattern to Display Framebuffer\n\r");
	// Then update existing options to 6 and 7
	xil_printf("6 - Invert Current Frame colors\n\r");
	xil_printf("7 - Invert Current Frame colors seamlessly\n\r");
	xil_printf("q - Quit\n\r");
	xil_printf("\n\r");
	xil_printf("\n\r");
	xil_printf("Enter a selection:");
}

void DemoChangeRes()
{
	int fResSet = 0;
	int status;
	char userInput = 0;

	/* Flush UART FIFO */
	while (XUartPs_IsReceiveData(UART_BASEADDR))
	{
		XUartPs_ReadReg(UART_BASEADDR, XUARTPS_FIFO_OFFSET);
	}

	while (!fResSet)
	{
		DemoCRMenu();

		/* Wait for data on UART */
		while (!XUartPs_IsReceiveData(UART_BASEADDR))
		{}

		/* Store the first character in the UART recieve FIFO and echo it */
		userInput = XUartPs_ReadReg(UART_BASEADDR, XUARTPS_FIFO_OFFSET);
		xil_printf("%c", userInput);
		status = XST_SUCCESS;
		switch (userInput)
		{
		case '1':
			status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_640x480);
			DisplayStart(&dispCtrl);
			fResSet = 1;
			break;
		case '2':
			status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_800x600);
			DisplayStart(&dispCtrl);
			fResSet = 1;
			break;
		case '3':
			status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_1280x720);
			DisplayStart(&dispCtrl);
			fResSet = 1;
			break;
		case '4':
			status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_1280x1024);
			DisplayStart(&dispCtrl);
			fResSet = 1;
			break;
		case '5':
			status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_1920x1080);
			DisplayStart(&dispCtrl);
			fResSet = 1;
			break;
		case 'q':
			fResSet = 1;
			break;
		default :
			xil_printf("\n\rInvalid Selection");
			TimerDelay(500000);
		}
		if (status == XST_DMA_ERROR)
		{
			xil_printf("\n\rWARNING: AXI VDMA Error detected and cleared\n\r");
		}
	}
}

void DemoCRMenu()
{
	xil_printf("\x1B[H"); //Set cursor to top left of terminal
	xil_printf("\x1B[2J"); //Clear terminal
	xil_printf("**************************************************\n\r");
	xil_printf("*               ZYBO Display Demo                *\n\r");
	xil_printf("**************************************************\n\r");
	xil_printf("*Current Resolution: %28s*\n\r", dispCtrl.vMode.label);
	printf("*Pixel Clock Freq. (MHz): %23.3f*\n\r", dispCtrl.pxlFreq);
	xil_printf("**************************************************\n\r");
	xil_printf("\n\r");
	xil_printf("1 - %s\n\r", VMODE_640x480.label);
	xil_printf("2 - %s\n\r", VMODE_800x600.label);
	xil_printf("3 - %s\n\r", VMODE_1280x720.label);
	xil_printf("4 - %s\n\r", VMODE_1280x1024.label);
	xil_printf("5 - %s\n\r", VMODE_1920x1080.label);
	xil_printf("q - Quit (don't change resolution)\n\r");
	xil_printf("\n\r");
	xil_printf("Select a new resolution:");
}

void DemoInvertFrame(u8 *srcFrame, u8 *destFrame, u32 width, u32 height, u32 stride)
{
	u32 xcoi, ycoi;
	u32 lineStart = 0;
	for(ycoi = 0; ycoi < height; ycoi++)
	{
		for(xcoi = 0; xcoi < (width * 4); xcoi+=4)
		{
			destFrame[xcoi + lineStart] = ~srcFrame[xcoi + lineStart];         //Red
			destFrame[xcoi + lineStart + 1] = ~srcFrame[xcoi + lineStart + 1]; //Blue
			destFrame[xcoi + lineStart + 2] = ~srcFrame[xcoi + lineStart + 2]; //Green
		}
		lineStart += stride;
	}
	/*
	 * Flush the framebuffer memory range to ensure changes are written to the
	 * actual memory, and therefore accessible by the VDMA.
	 */
	Xil_DCacheFlushRange((unsigned int) destFrame, DEMO_MAX_FRAME);
}

void DemoPrintTest(u8 *frame, u32 width, u32 height, u32 stride, int pattern)
{
	u32 xcoi, ycoi;
	u32 iPixelAddr;
	u8 wRed, wBlue, wGreen;
	u32 wCurrentInt;
	double fRed, fBlue, fGreen, fColor;
	u32 xLeft, xMid, xRight, xInt;
	u32 yMid, yInt;
	double xInc, yInc;


	switch (pattern)
	{
	case DEMO_PATTERN_0:

		xInt = width / 4; //Four intervals, each with width/4 pixels
		xLeft = xInt * 3;
		xMid = xInt * 2 * 3;
		xRight = xInt * 3 * 3;
		xInc = 256.0 / ((double) xInt); //256 color intensities are cycled through per interval (overflow must be caught when color=256.0)

		yInt = height / 2; //Two intervals, each with width/2 lines
		yMid = yInt;
		yInc = 256.0 / ((double) yInt); //256 color intensities are cycled through per interval (overflow must be caught when color=256.0)

		fBlue = 0.0;
		fRed = 256.0;
		for(xcoi = 0; xcoi < (width*4); xcoi+=4)
		{
			/*
			 * Convert color intensities to integers < 256, and trim values >=256
			 */
			wRed = (fRed >= 256.0) ? 255 : ((u8) fRed);
			wBlue = (fBlue >= 256.0) ? 255 : ((u8) fBlue);
			iPixelAddr = xcoi;
			fGreen = 0.0;
			for(ycoi = 0; ycoi < height; ycoi++)
			{

				wGreen = (fGreen >= 256.0) ? 255 : ((u8) fGreen);
				frame[iPixelAddr] = wRed;
				frame[iPixelAddr + 1] = wBlue;
				frame[iPixelAddr + 2] = wGreen;
				if (ycoi < yMid)
				{
					fGreen += yInc;
				}
				else
				{
					fGreen -= yInc;
				}

				/*
				 * This pattern is printed one vertical line at a time, so the address must be incremented
				 * by the stride instead of just 1.
				 */
				iPixelAddr += stride;
			}

			if (xcoi < xLeft)
			{
				fBlue = 0.0;
				fRed -= xInc;
			}
			else if (xcoi < xMid)
			{
				fBlue += xInc;
				fRed += xInc;
			}
			else if (xcoi < xRight)
			{
				fBlue -= xInc;
				fRed -= xInc;
			}
			else
			{
				fBlue += xInc;
				fRed = 0;
			}
		}
		/*
		 * Flush the framebuffer memory range to ensure changes are written to the
		 * actual memory, and therefore accessible by the VDMA.
		 */
		Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);
		break;
	case DEMO_PATTERN_1:

		xInt = width / 7; //Seven intervals, each with width/7 pixels
		xInc = 256.0 / ((double) xInt); //256 color intensities per interval. Notice that overflow is handled for this pattern.

		fColor = 0.0;
		wCurrentInt = 1;
		for(xcoi = 0; xcoi < (width*4); xcoi+=4)
		{

			/*
			 * Just draw white in the last partial interval (when width is not divisible by 7)
			 */
			if (wCurrentInt > 7)
			{
				wRed = 255;
				wBlue = 255;
				wGreen = 255;
			}
			else
			{
				if (wCurrentInt & 0b001)
					wRed = (u8) fColor;
				else
					wRed = 0;

				if (wCurrentInt & 0b010)
					wBlue = (u8) fColor;
				else
					wBlue = 0;

				if (wCurrentInt & 0b100)
					wGreen = (u8) fColor;
				else
					wGreen = 0;
			}

			iPixelAddr = xcoi;

			for(ycoi = 0; ycoi < height; ycoi++)
			{
				frame[iPixelAddr] = wRed;
				frame[iPixelAddr + 1] = wBlue;
				frame[iPixelAddr + 2] = wGreen;
				/*
				 * This pattern is printed one vertical line at a time, so the address must be incremented
				 * by the stride instead of just 1.
				 */
				iPixelAddr += stride;
			}

			fColor += xInc;
			if (fColor >= 256.0)
			{
				fColor = 0.0;
				wCurrentInt++;
			}
		}
		/*
		 * Flush the framebuffer memory range to ensure changes are written to the
		 * actual memory, and therefore accessible by the VDMA.
		 */
		Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);
		break;

	default :
		xil_printf("Error: invalid pattern passed to DemoPrintTest");
	}
}


