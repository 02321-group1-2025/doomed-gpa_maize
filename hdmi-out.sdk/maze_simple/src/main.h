#ifndef SRC_MAIN_H_
#define SRC_MAIN_H_

#include "xparameters.h"
#include "labyrinth.h"
/* Configuration constants */

/* Hardware constants from xparameters.h */
#define DYNCLK_BASEADDR XPAR_AXI_DYNCLK_0_BASEADDR
#define VGA_VDMA_ID     XPAR_AXIVDMA_0_DEVICE_ID
#define DISP_VTC_ID     XPAR_VTC_0_DEVICE_ID
#define SCU_TIMER_ID    XPAR_SCUTIMER_DEVICE_ID
#define UART_BASEADDR XPAR_PS7_UART_1_BASEADDR


/* Function prototypes */
void initialize_display(void);
void draw_frame(void);
void draw_maze(uint8_t *maze, uint8_t *frame_buf, uint8_t *maze_buf);
void generate_maze_buffer(maze_t *maze, uint8_t *maze_buf);


#endif /* SRC_MAIN_H_ */
