#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "qoi.h"
#include "video_frames.h"

uint32_t swap_bytes(uint32_t value) {
    return ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) | ((value & 0xFF000000) >> 24);
}

qoi_header_t qoi_decode_header(const unsigned char* data) {
   qoi_header_t qoi_header;
   memcpy(&qoi_header, data, sizeof(qoi_header_t));

   // Swap bytes to handle endianness
   qoi_header.width = swap_bytes(qoi_header.width);
   qoi_header.height = swap_bytes(qoi_header.height);

   return qoi_header;
}

void qoi_decode(const unsigned char* data, size_t size, RGBa_t *pix_buf, qoi_header_t *qoi_header) {
   const unsigned char* ptr = data + sizeof(qoi_header_t);

   RGBa_t previous_pixel = {
      .red     =   0,
      .green   =   0,
      .blue    =   0,
      .alpha   = 255,
   };
   RGBa_t previous_pixels[64];

   for (uint64_t i = 0; i < (qoi_header->width*qoi_header->height); i++) {
      uint8_t data;
      memcpy(&data, ptr, sizeof(uint8_t));
      ptr += sizeof(uint8_t);

      RGBa_t current_pixel;

      switch (data) {
         // QOI_OP_RGB
         case 0b11111110: {
            DEBUG_PRINT("QOI_OP_RGB");
            memcpy(&current_pixel, ptr, sizeof(RGBa_t)-1);
            ptr += sizeof(RGBa_t)-1;
            current_pixel.alpha = previous_pixel.alpha;

            DEBUG_PRINT("(r: %d, g: %d, b: %d, a: %d)", current_pixel.red, current_pixel.blue, current_pixel.green, current_pixel.alpha);

            break;
         }

         // QOI_OP_RGBA
         case 0b11111111: {
            DEBUG_PRINT("QOI_OP_RGBA - ");
            memcpy(&current_pixel, ptr, sizeof(RGBa_t));
            ptr += sizeof(RGBa_t);

            DEBUG_PRINT("(r: %d, g: %d, b: %d, a: %d)", current_pixel.red, current_pixel.blue, current_pixel.green, current_pixel.alpha);

            break;
         }

         default: {
            uint8_t tag = (data & 0xC0) >> 6;

            switch (tag) {
               // QOI_OP_INDEX
               case 0b00: {
                  DEBUG_PRINT("QOI_OP_INDEX - ");

                  uint8_t index = data & 0b00111111;
                  current_pixel = previous_pixels[index];

                  DEBUG_PRINT("index: %d", index);

                  break;
               }

               // QOI_OP_DIFF
               case 0b01: {
                  DEBUG_PRINT("QOI_OP_DIFF - ");

                  int8_t dr = (data & 0b00110000) >> 4;
                  int8_t dg = (data & 0b00001100) >> 2;
                  int8_t db = (data & 0b00000011) >> 0;

                  // Stored with a bias of 2
                  dr -= 2;
                  dg -= 2;
                  db -= 2;

                  DEBUG_PRINT("(dr: %d, dg: %d, db: %d) ", dr, dg, db);

                  current_pixel.red = previous_pixel.red + dr;
                  current_pixel.green = previous_pixel.green + dg;
                  current_pixel.blue = previous_pixel.blue + db;
                  current_pixel.alpha = previous_pixel.alpha;

                  break;
               }

               // QOI_OP_LUMA
               case 0b10: {
                  DEBUG_PRINT("QOI_OP_LUMA - ");
                  uint8_t data2;
                  memcpy(&data2, ptr, sizeof(uint8_t));
                  ptr += sizeof(uint8_t);

                  int8_t diff_green = data & 0b00111111;
                  diff_green -= 32;

                  int8_t dr_dg = (data2 & 0xF0) >> 4;
                  int8_t db_dg = (data2 & 0x0F) >> 0;

                  dr_dg -= 8;
                  db_dg -= 8;

                  uint8_t diff_red = dr_dg + diff_green;
                  uint8_t diff_blue = db_dg + diff_green;

                  DEBUG_PRINT("Prev: (%d, %d, %d, %d)", previous_pixel.red, previous_pixel.green, previous_pixel.blue, previous_pixel.alpha);

                  current_pixel.red = previous_pixel.red + diff_red;
                  current_pixel.green = previous_pixel.green + diff_green;
                  current_pixel.blue = previous_pixel.blue + diff_blue;
                  current_pixel.alpha = previous_pixel.alpha;

                  DEBUG_PRINT("(r: %d, g: %d, b: %d, a: %d)", current_pixel.red, current_pixel.blue, current_pixel.green, current_pixel.alpha);
                  break;
               }

               // QOI_OP_RUN
               case 0b11 : {
                  DEBUG_PRINT("QOI_OP_RUN - ");
                  uint8_t run = data & 0b00111111;
                  run += 1; // Bias of -1
                  DEBUG_PRINT("%d", run);

                  if (run == 63 || run == 64) {
                     //err(1, "Panic! Illegal value for run-length: %d\n",run);
                     printf("Panic! Illegal value for run-length: %d\n", run);
                     return;
                  }

                  current_pixel = previous_pixel;

                  for (uint32_t j = i; j < i+(run-1); j++) {
                     pix_buf[j] = current_pixel;
                  }

                  i += run - 1;

                  break;
               }
            }
         }
      }

      pix_buf[i] = current_pixel;
      DEBUG_PRINT(" - (r: %d, g: %d, b: %d, a: %d)", current_pixel.red, current_pixel.green, current_pixel.blue, current_pixel.alpha);

      uint8_t pixel_index = index_position(current_pixel.red,
                                           current_pixel.green,
                                           current_pixel.blue,
                                           current_pixel.alpha
                                          );
      DEBUG_PRINT(" - index : %d\n", pixel_index);
      previous_pixels[pixel_index] = current_pixel;
      previous_pixel = current_pixel;
   }
}
