#include <stdint.h>
#include <stdlib.h>

#include "display.h"

void line(uint8_t* frame, uint8_t red, uint8_t green, uint8_t blue, int16_t x0, int16_t y0, int16_t x1, int16_t y1){
	if(x0 < 0 || y0 < 0 || x1 < 0 || y1 < 0) return;
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

void rect(uint8_t* frame, uint8_t red, uint8_t green, uint8_t blue, int x0, int y0, int width, int height, uint8_t fill){
	if (fill) {
	    for (int x = x0; x < x0 + width; x++) {
	        for (int y = y0; y < y0 + height; y++) {
	            PIXEL(frame,red,green,blue,x,y);
	        }
	    }
	} else {
	    for (int x = x0; x < x0 + width; x++) {

	    	PIXEL(frame, red, green, blue, x, y0         );//top
			PIXEL(frame, red, green, blue, x, y0 + height - 1);//bottom
	    }
	    for (int y = y0; y < y0 + height; y++) {
	        PIXEL(frame, red, green, blue, x0        , y );//left
	        PIXEL(frame, red, green, blue, x0 + width - 1, y );//right
	    }
	}
}

void rectCenter(uint8_t* frame, uint8_t red, uint8_t green, uint8_t blue, int x0, int y0, int width, int height, uint8_t fill){
	rect(frame,red,green,blue,x0-(width/2),y0-(height/2),width,height,fill);
}
