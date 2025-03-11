#include <stdio.h>
#include <stdint.h>

#define index_position(r, g, b, a) ((r*3 + g*5 + b*7 + a*11) % 64)
#define index_position_2(rgba) ((rgba.red*3 + rgba.green*5 + rgba.blue*7 + rgba.alpha*11) % 64) // NB: This shit prolly no worky

#ifdef DEBUG
   #define DEBUG_PRINT(fmt, args...) fprintf(stderr, "DEBUG: %s:%d:%s() - " fmt, __FILE__, __LINE__, __FUNCTION__, ##args)
#elif defined DEBUG_SIMPLE
   #define DEBUG_PRINT(fmt, args...) fprintf(stderr, fmt, ##args)
#else
   #define DEBUG_PRINT(fmt, args...) /* Don't do anything on release builds */
#endif /* ifdef  DEBUG */

typedef struct __attribute__((packed)) qoi_header {
   char        magic[4];   // 4 byte magic number "qoif"
   uint32_t    width;      // Big endian
   uint32_t    height;     // Big endian
   uint8_t     channels;   // 3 = RGB, 4 = RGBA
   uint8_t     colorspace; // 0 = sRGB linear alpha
                           // 1 = all channels linear
} qoi_header_t;

typedef struct __attribute((__packed__)) RGBa {
   uint8_t blue;
   uint8_t green;
   uint8_t red;
   uint8_t alpha;
} RGBa_t;

qoi_header_t qoi_decode_header(const unsigned char* data);
void qoi_decode(const unsigned char* data, size_t size, RGBa_t *pix_buf, qoi_header_t *qoi_header);
