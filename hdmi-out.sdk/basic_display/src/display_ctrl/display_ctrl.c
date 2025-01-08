/************************************************************************/
/*																		*/
/*	display_ctrl.c	--	Digilent Display Controller Driver				*/
/*																		*/
/************************************************************************/
/*	Author: Sam Bobrowicz												*/
/*	Copyright 2014, Digilent Inc.										*/
/************************************************************************/
/*  Module Description: 												*/
/*																		*/
/*		This module provides an easy to use API for controlling a    	*/
/*		Display attached to a Digilent system board via VGA or HDMI. 	*/
/*		run-time resolution setting and seamless framebuffer-swapping 	*/
/*		for tear-free animation. 										*/
/*																		*/
/*		To use this driver, you must have a Xilinx Video Timing 		*/
/* 		Controller core (vtc), Xilinx axi_vdma core, a Digilent 		*/
/*		axi_dynclk core, a Xilinx AXI Stream to Video core, and either  */
/*		a Digilent RGB2VGA or RGB2DVI core all present in your design.  */
/*		See the Video in or Display out reference projects for your     */
/*		system board to see how they need to be connected. Digilent     */
/*		reference projects and IP cores can be found at 				*/
/*		www.github.com/Digilent.			 							*/
/*																		*/
/*		The following steps should be followed to use this driver:		*/
/*		1) Create a DisplayCtrl object and pass a pointer to it to 		*/
/*		   DisplayInitialize.											*/
/*		2) Call DisplaySetMode to set the desired mode					*/
/*		3) Call DisplayStart to begin outputting data to the display	*/
/*		4) To create a seamless animation, draw the next image to a		*/
/*		   framebuffer currently not being displayed. Then call 		*/
/*		   DisplayChangeFrame to begin displaying that frame.			*/
/*		   Repeat as needed, only ever modifying inactive frames.		*/
/*		5) To change the resolution, call DisplaySetMode, followed by	*/
/*		   DisplayStart again.											*/
/*																		*/
/*																		*/
/************************************************************************/
/*  Revision History:													*/
/* 																		*/
/*		2/20/2014(SamB): Created										*/
/*		11/25/2015(SamB): Changed from axi_dispctrl to Xilinx cores		*/
/*						  Separated Clock functions into dynclk library */
/*																		*/
/************************************************************************/
/*
 * TODO: It would be nice to remove the need for users above this to access
 *       members of the DisplayCtrl struct manually. This can be done by
 *       implementing get/set functions for things like video mode, state,
 *       etc.
 */


/* ------------------------------------------------------------ */
/*				Include File Definitions						*/
/* ------------------------------------------------------------ */

/*
 * Uncomment for Debugging messages over UART
 */
//#define DEBUG

#include "display_ctrl.h"
#include "xdebug.h"
#include "xil_io.h"

/* ------------------------------------------------------------ */
/*				Procedure Definitions							*/
/* ------------------------------------------------------------ */

/***	DisplayStop(DisplayCtrl *dispPtr)
**
**	Parameters:
**		dispPtr - Pointer to the initialized DisplayCtrl struct
**
**	Return Value: int
**		XST_SUCCESS if successful.
**		XST_DMA_ERROR if an error was detected on the DMA channel. The
**			Display is still successfully stopped, and the error is
**			cleared so that subsequent DisplayStart calls will be
**			successful. This typically indicates insufficient bandwidth
**			on the AXI Memory-Map Interconnect (VDMA<->DDR)
**
**	Description:
**		Halts output to the display
**
*/
int DisplayStop(DisplayCtrl *dispPtr)
{
	/*
	 * If already stopped, do nothing
	 */
	if (dispPtr->state == DISPLAY_STOPPED)
	{
		return XST_SUCCESS;
	}

	/*
	 * Disable the disp_ctrl core, and wait for the current frame to finish (the core cannot stop
	 * mid-frame)
	 */
	XVtc_DisableGenerator(&dispPtr->vtc);

	/*
	 * Stop the VDMA core
	 */
	XAxiVdma_DmaStop(dispPtr->vdma, XAXIVDMA_READ);
	while(XAxiVdma_IsBusy(dispPtr->vdma, XAXIVDMA_READ));

	/*
	 * Update Struct state
	 */
	dispPtr->state = DISPLAY_STOPPED;

	//TODO: consider stopping the clock here, perhaps after a check to see if the VTC is finished

	if (XAxiVdma_GetDmaChannelErrors(dispPtr->vdma, XAXIVDMA_READ))
	{
		xdbg_printf(XDBG_DEBUG_GENERAL, "Clearing DMA errors...\r\n");
		XAxiVdma_ClearDmaChannelErrors(dispPtr->vdma, XAXIVDMA_READ, 0xFFFFFFFF);
		return XST_DMA_ERROR;
	}

	return XST_SUCCESS;
}
/* ------------------------------------------------------------ */

/***	DisplayStart(DisplayCtrl *dispPtr)
**
**	Parameters:
**		dispPtr - Pointer to the initialized DisplayCtrl struct
**
**	Return Value: int
**		XST_SUCCESS if successful, XST_FAILURE otherwise
**
**	Errors:
**
**	Description:
**		Starts the display.
**
*/
int
DisplayStart(DisplayCtrl *disp_ptr)
{
    int status;
    ClkConfig clk_reg;
    ClkMode clk_mode;
    XVtc_Timing vtc_timing;
    XVtc_SourceSelect source_select;

    xdbg_printf(XDBG_DEBUG_GENERAL, "display start entered\n\r");

    /* If already started, do nothing */
    if (disp_ptr->state == DISPLAY_RUNNING) {
        return XST_SUCCESS;
    }

    /* Configure the pixel clock - this sets up our display timing */
    ClkFindParams(disp_ptr->vMode.freq, &clk_mode);
    disp_ptr->pxlFreq = clk_mode.freq;  // Store actual frequency achieved

    if (!ClkFindReg(&clk_reg, &clk_mode)) {
        xdbg_printf(XDBG_DEBUG_GENERAL, "Error calculating CLK register values\n\r");
        return XST_FAILURE;
    }

    /* Restart the clock with new parameters */
    ClkWriteReg(&clk_reg, disp_ptr->dynClkAddr);
    ClkStop(disp_ptr->dynClkAddr);
    ClkStart(disp_ptr->dynClkAddr);

    /* Configure the VTC timing for our video mode */
    vtc_timing.HActiveVideo = disp_ptr->vMode.width;
    vtc_timing.HFrontPorch = disp_ptr->vMode.hps - disp_ptr->vMode.width;
    vtc_timing.HSyncWidth = disp_ptr->vMode.hpe - disp_ptr->vMode.hps;
    vtc_timing.HBackPorch = disp_ptr->vMode.hmax - disp_ptr->vMode.hpe + 1;
    vtc_timing.HSyncPolarity = disp_ptr->vMode.hpol;

    vtc_timing.VActiveVideo = disp_ptr->vMode.height;
    vtc_timing.V0FrontPorch = disp_ptr->vMode.vps - disp_ptr->vMode.height;
    vtc_timing.V0SyncWidth = disp_ptr->vMode.vpe - disp_ptr->vMode.vps;
    vtc_timing.V0BackPorch = disp_ptr->vMode.vmax - disp_ptr->vMode.vpe + 1;
    /* Copy vertical timing to both fields (we're not using interlacing) */
    vtc_timing.V1FrontPorch = vtc_timing.V0FrontPorch;
    vtc_timing.V1SyncWidth = vtc_timing.V0SyncWidth;
    vtc_timing.V1BackPorch = vtc_timing.V0BackPorch;
    vtc_timing.VSyncPolarity = disp_ptr->vMode.vpol;
    vtc_timing.Interlaced = 0;  // Progressive scan

    /* Configure VTC source select - use generator timing */
    memset(&source_select, 0, sizeof(source_select));
    source_select.VBlankPolSrc = 1;
    source_select.VSyncPolSrc = 1;
    source_select.HBlankPolSrc = 1;
    source_select.HSyncPolSrc = 1;
    source_select.ActiveVideoPolSrc = 1;
    source_select.ActiveChromaPolSrc = 1;
    source_select.VChromaSrc = 1;
    source_select.VActiveSrc = 1;
    source_select.VBackPorchSrc = 1;
    source_select.VSyncSrc = 1;
    source_select.VFrontPorchSrc = 1;
    source_select.VTotalSrc = 1;
    source_select.HActiveSrc = 1;
    source_select.HBackPorchSrc = 1;
    source_select.HSyncSrc = 1;
    source_select.HFrontPorchSrc = 1;
    source_select.HTotalSrc = 1;

    /* Initialize and start the VTC */
    XVtc_RegUpdateEnable(&(disp_ptr->vtc));
    XVtc_SetGeneratorTiming(&(disp_ptr->vtc), &vtc_timing);
    XVtc_SetSource(&(disp_ptr->vtc), &source_select);
    XVtc_EnableGenerator(&disp_ptr->vtc);

    /* Configure the VDMA channel for the current mode */
    disp_ptr->vdmaConfig.VertSizeInput = disp_ptr->vMode.height;
    disp_ptr->vdmaConfig.HoriSizeInput = disp_ptr->vMode.width * 4;  // 4 bytes per pixel
    disp_ptr->vdmaConfig.Stride = disp_ptr->stride;
    disp_ptr->vdmaConfig.FrameStoreStartAddr[0] = (u32)disp_ptr->frame_ptr;

    /* Start the VDMA engine */
    status = XAxiVdma_DmaConfig(disp_ptr->vdma, XAXIVDMA_READ, &(disp_ptr->vdmaConfig));
    if (status != XST_SUCCESS) {
        xdbg_printf(XDBG_DEBUG_GENERAL, "Read channel config failed %d\r\n", status);
        return XST_FAILURE;
    }

    status = XAxiVdma_DmaSetBufferAddr(disp_ptr->vdma, XAXIVDMA_READ,
                                      disp_ptr->vdmaConfig.FrameStoreStartAddr);
    if (status != XST_SUCCESS) {
        xdbg_printf(XDBG_DEBUG_GENERAL, "Read channel set buffer address failed %d\r\n", status);
        return XST_FAILURE;
    }

    status = XAxiVdma_DmaStart(disp_ptr->vdma, XAXIVDMA_READ);
    if (status != XST_SUCCESS) {
        xdbg_printf(XDBG_DEBUG_GENERAL, "Start read transfer failed %d\r\n", status);
        return XST_FAILURE;
    }

    disp_ptr->state = DISPLAY_RUNNING;
    return XST_SUCCESS;
}

/* ------------------------------------------------------------ */

/***	DisplayInitialize(DisplayCtrl *dispPtr, XAxiVdma *vdma, u16 vtcId, u32 dynClkAddr, u8 *framePtr[DISPLAY_NUM_FRAMES], u32 stride)
**
**	Parameters:
**		dispPtr - Pointer to the struct that will be initialized
**		vdma - Pointer to initialized VDMA struct
**		vtcId - Device ID of the VTC core as found in xparameters.h
**		dynClkAddr - BASE ADDRESS of the axi_dynclk core
**		framePtr - array of pointers to the framebuffers. The framebuffers must be instantiated above this driver, and there must be 3
**		stride - line stride of the framebuffers. This is the number of bytes between the start of one line and the start of another.
**
**	Return Value: int
**		XST_SUCCESS if successful, XST_FAILURE otherwise
**
**	Errors:
**
**	Description:
**		Initializes the driver struct for use.
**
*/
//int DisplayInitialize(DisplayCtrl *dispPtr, XAxiVdma *vdma, u16 vtcId, u32 dynClkAddr, u8 *framePtr[DISPLAY_NUM_FRAMES], u32 stride)
//{
//	int Status;
//	int i;
//	XVtc_Config *vtcConfig;
//	ClkConfig clkReg;
//	ClkMode clkMode;
//
//
//	/*
//	 * Initialize all the fields in the DisplayCtrl struct
//	 */
//	dispPtr->curFrame = 0;
//	dispPtr->dynClkAddr = dynClkAddr;
//	for (i = 0; i < DISPLAY_NUM_FRAMES; i++)
//	{
//		dispPtr->framePtr[i] = framePtr[i];
//	}
//	dispPtr->state = DISPLAY_STOPPED;
//	dispPtr->stride = stride;
//	dispPtr->vMode = VMODE_640x480;
//
//	ClkFindParams(dispPtr->vMode.freq, &clkMode);
//
//	/*
//	 * Store the obtained frequency to pxlFreq. It is possible that the PLL was not able to
//	 * exactly generate the desired pixel clock, so this may differ from vMode.freq.
//	 */
//	dispPtr->pxlFreq = clkMode.freq;
//
//	/*
//	 * Write to the PLL dynamic configuration registers to configure it with the calculated
//	 * parameters.
//	 */
//	if (!ClkFindReg(&clkReg, &clkMode))
//	{
//		xdbg_printf(XDBG_DEBUG_GENERAL, "Error calculating CLK register values\n\r");
//		return XST_FAILURE;
//	}
//	ClkWriteReg(&clkReg, dispPtr->dynClkAddr);
//
//	/*
//	 * Enable the dynamically generated clock
//    */
//	ClkStart(dispPtr->dynClkAddr);
//
//	/* Initialize the VTC driver so that it's ready to use look up
//	 * configuration in the config table, then initialize it.
//	 */
//	vtcConfig = XVtc_LookupConfig(vtcId);
//	/* Checking Config variable */
//	if (NULL == vtcConfig) {
//		return (XST_FAILURE);
//	}
//	Status = XVtc_CfgInitialize(&(dispPtr->vtc), vtcConfig, vtcConfig->BaseAddress);
//	/* Checking status */
//	if (Status != (XST_SUCCESS)) {
//		return (XST_FAILURE);
//	}
//
//	dispPtr->vdma = vdma;
//
//
//	/*
//	 * Initialize the VDMA Read configuration struct
//	 */
//	dispPtr->vdmaConfig.FrameDelay = 0;
//	dispPtr->vdmaConfig.EnableCircularBuf = 1;
//	dispPtr->vdmaConfig.EnableSync = 0;
//	dispPtr->vdmaConfig.PointNum = 0;
//	dispPtr->vdmaConfig.EnableFrameCounter = 0;
//
//	return XST_SUCCESS;
//}
int
DisplayInitialize(DisplayCtrl *disp_ptr, XAxiVdma *vdma, u16 vtc_id,
                 u32 dynclk_addr, u8 *frame_ptr, u32 stride)
{
    int status;
    XVtc_Config *vtc_config;
    ClkConfig clk_reg;
    ClkMode clk_mode;

    /*
     * Initialize all the fields in the DisplayCtrl struct
     */
    disp_ptr->dynClkAddr = dynclk_addr;
    disp_ptr->frame_ptr = frame_ptr;
    disp_ptr->state = DISPLAY_STOPPED;
    disp_ptr->stride = stride;
    disp_ptr->vMode = VMODE_640x480;

    ClkFindParams(disp_ptr->vMode.freq, &clk_mode);
    disp_ptr->pxlFreq = clk_mode.freq;

    if (!ClkFindReg(&clk_reg, &clk_mode)) {
        xil_printf("Error calculating CLK register values\r\n");
        return XST_FAILURE;
    }
    ClkWriteReg(&clk_reg, disp_ptr->dynClkAddr);
    ClkStart(disp_ptr->dynClkAddr);

    /* Initialize the VTC driver */
    vtc_config = XVtc_LookupConfig(vtc_id);
    if (NULL == vtc_config) {
        return XST_FAILURE;
    }
    status = XVtc_CfgInitialize(&(disp_ptr->vtc), vtc_config,
                               vtc_config->BaseAddress);
    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    disp_ptr->vdma = vdma;

    /*
     * Initialize the VDMA Read configuration struct
     */
    disp_ptr->vdmaConfig.FrameDelay = 0;
    disp_ptr->vdmaConfig.EnableCircularBuf = 0;  // Changed to 0 since we're not cycling frames
    disp_ptr->vdmaConfig.EnableSync = 0;
    disp_ptr->vdmaConfig.PointNum = 0;
    disp_ptr->vdmaConfig.EnableFrameCounter = 0;

    return XST_SUCCESS;
}
/* ------------------------------------------------------------ */

/***	DisplaySetMode(DisplayCtrl *dispPtr, const VideoMode *newMode)
**
**	Parameters:
**		dispPtr - Pointer to the initialized DisplayCtrl struct
**		newMode - The VideoMode struct describing the new mode.
**
**	Return Value: int
**		XST_SUCCESS if successful, XST_FAILURE otherwise
**
**	Errors:
**
**	Description:
**		Changes the resolution being output to the display. If the display
**		is currently started, it is automatically stopped (DisplayStart must
**		be called again).
**
*/
int DisplaySetMode(DisplayCtrl *dispPtr, const VideoMode *newMode)
{
	int Status;

	/*
	 * If currently running, stop
	 */
	if (dispPtr->state == DISPLAY_RUNNING)
	{
		Status = DisplayStop(dispPtr);
		if (Status != XST_SUCCESS)
		{
			xdbg_printf(XDBG_DEBUG_GENERAL, "Cannot change mode, unable to stop display %d\r\n", Status);
			return XST_FAILURE;
		}
	}

	dispPtr->vMode = *newMode;

	return XST_SUCCESS;
}
/* ------------------------------------------------------------ */



/************************************************************************/

