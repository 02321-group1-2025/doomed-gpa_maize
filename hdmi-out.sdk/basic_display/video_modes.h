typedef struct {
	char label[64]; /* Label describing the resolution */
	u32 width; /*Width of the active video frame*/
	u32 height; /*Height of the active video frame*/
	u32 hps; /*Start time of Horizontal sync pulse, in pixel clocks (active width + H. front porch)*/
	u32 hpe; /*End time of Horizontal sync pulse, in pixel clocks (active width + H. front porch + H. sync width)*/
	u32 hmax; /*Total number of pixel clocks per line (active width + H. front porch + H. sync width + H. back porch) */
	u32 hpol; /*hsync pulse polarity*/
	u32 vps; /*Start time of Vertical sync pulse, in lines (active height + V. front porch)*/
	u32 vpe; /*End time of Vertical sync pulse, in lines (active height + V. front porch + V. sync width)*/
	u32 vmax; /*Total number of lines per frame (active height + V. front porch + V. sync width + V. back porch) */
	u32 vpol; /*vsync pulse polarity*/
	double freq; /*Pixel Clock frequency*/
} VideoMode;

static const VideoMode VMODE_640x480 = {
	.label = "640x480@60Hz",
	.width = 640,
	.height = 480,
	.hps = 656,
	.hpe = 752,
	.hmax = 799,
	.hpol = 0,
	.vps = 490,
	.vpe = 492,
	.vmax = 524,
	.vpol = 0,
	.freq = 25.0
};
