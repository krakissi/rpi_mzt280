#define DOTTED_DRAW_SIZE (DISP_W * DISP_H / 2)
#define FRAMEBUFFER_INVERT_VIDEO 0x40

typedef struct framebuffer {
	int drawmap[2][DISP_H][DISP_W];
	int diffmap[2][DOTTED_DRAW_SIZE];
	int w, h, flag, mode, invert;
	unsigned char *buffer;
	FILE *infile;
} framebuffer;

framebuffer *framebuffer_create(int mode, const char *infilestr){
	framebuffer *fb = malloc(sizeof(struct framebuffer));
	FILE *infile = fopen(infilestr, "rb");
	int i, j;

	// Get the invert flag from the display mode and then clear it out.
	fb->invert = mode & FRAMEBUFFER_INVERT_VIDEO;
	mode &= ~FRAMEBUFFER_INVERT_VIDEO;

	fb->w = 2 * DISP_W;
	fb->h = 2 * DISP_H;
	fb->mode = mode;
	switch(mode){
		// 320x240 native mode
		case 2:
			fb->w = DISP_W;
			fb->h = DISP_H;
			break;

		// 640x480 single-sample mode (very lossy)
		case 3:
			break;

		// 640x480 interlaced mode
		case 4:
		case 5:
			break;

		// 640x480 checkboard mode
		case 6:
		case 7:
			break;

		// 640x480 downsample mode
		case 1:
		default:
			fb->mode = 1;
	}
	fb->buffer = malloc(fb->w * fb->h * 2);
	fb->infile = infile;
	fb->flag = 1;

	for(i = 0; i <= MAX_Y; i++){
		for(j = 0; j <= MAX_X; j++){
			fb->drawmap[0][i][j] = 0;
			LCD_WR_Data(0);
			fb->drawmap[1][i][j] = 255;
		}
	}

	return fb;
}

void framebuffer_read(framebuffer *fb){
	fseek(fb->infile, 0, 0);
	fread(fb->buffer, fb->w * fb->h * 2, sizeof(unsigned char), fb->infile);
}
