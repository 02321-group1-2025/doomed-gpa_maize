#pragma once
#ifndef DISPLAY_H
#define DISPLAY_H

/* Display configuration */
#define DISPLAY_WIDTH  640
#define DISPLAY_HEIGHT 480
#define DISPLAY_STRIDE (DISPLAY_WIDTH * 4)  // 4 bytes per pixel (BGRA)
#define FRAME_SIZE     (DISPLAY_WIDTH * DISPLAY_HEIGHT * 4)


/* Pixel access macros */
#define FRAME_PIXEL(x, y)         ((y) * DISPLAY_STRIDE + (x) * 4)
#define FRAME_PIXEL_B(frame, x, y) frame[FRAME_PIXEL(x, y) + 0]  // Blue is offset 0
#define FRAME_PIXEL_G(frame, x, y) frame[FRAME_PIXEL(x, y) + 1]  // Green is offset 1
#define FRAME_PIXEL_R(frame, x, y) frame[FRAME_PIXEL(x, y) + 2]  // Red is offset 2
#define FRAME_PIXEL_A(frame, x, y) frame[FRAME_PIXEL(x, y) + 3]  // Alpha is offset 3

#define PIXEL(frame, red, green, blue, x, y) {\
	FRAME_PIXEL_R(frame, x, y) = red;\
	FRAME_PIXEL_G(frame, x, y) = green;\
	FRAME_PIXEL_B(frame, x, y) = blue;\
}
#define POINT(frame, red, green, blue, x, y) PIXEL(frame, red, green, blue, x, y)

#endif /* DISPLAY_H */
