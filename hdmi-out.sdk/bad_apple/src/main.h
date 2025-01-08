#ifndef SRC_MAIN_H_
#define SRC_MAIN_H_

#include "xparameters.h"

/* Configuration constants */
#define DISPLAY_WIDTH  640
#define DISPLAY_HEIGHT 480
#define DISPLAY_STRIDE (DISPLAY_WIDTH * 4)  // 4 bytes per pixel (BGRA)
#define FRAME_SIZE     (DISPLAY_WIDTH * DISPLAY_HEIGHT * 4)

/* Hardware constants from xparameters.h */
#define DYNCLK_BASEADDR XPAR_AXI_DYNCLK_0_BASEADDR
#define VGA_VDMA_ID     XPAR_AXIVDMA_0_DEVICE_ID
#define DISP_VTC_ID     XPAR_VTC_0_DEVICE_ID
#define SCU_TIMER_ID    XPAR_SCUTIMER_DEVICE_ID

/* Pixel access macros */
#define FRAME_PIXEL(x, y)         ((y) * DISPLAY_STRIDE + (x) * 4)
#define FRAME_PIXEL_B(frame, x, y) frame[FRAME_PIXEL(x, y) + 0]  // Blue is offset 0
#define FRAME_PIXEL_G(frame, x, y) frame[FRAME_PIXEL(x, y) + 1]  // Green is offset 1
#define FRAME_PIXEL_R(frame, x, y) frame[FRAME_PIXEL(x, y) + 2]  // Red is offset 2
#define FRAME_PIXEL_A(frame, x, y) frame[FRAME_PIXEL(x, y) + 3]  // Alpha is offset 3

/* Function prototypes */
void initialize_display(void);
void draw_frame(void);

#endif /* SRC_MAIN_H_ */
