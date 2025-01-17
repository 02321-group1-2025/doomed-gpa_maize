#pragma once
#ifndef DRAW_H
#define DRAW_H

void line(uint8_t* frame, uint8_t red, uint8_t green, uint8_t blue, int x0, int y0, int x1, int y1);
void rect(uint8_t* frame, uint8_t red, uint8_t green, uint8_t blue, int x0, int y0, int width, int height, uint8_t fill);
void rectCenter(uint8_t* frame, uint8_t red, uint8_t green, uint8_t blue, int x0, int y0, int width, int height, uint8_t fill);

#endif
