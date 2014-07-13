typedef struct framebuffer {
	unsigned char *buffer;
	int w, h;
	FILE *infile;
	long fp;
	int drawmap[2][240][320];
	int diffmap[240][320];
	int flag;
	int mode;
} framebuffer;

framebuffer *framebuffer_create(int mode, const char *infilestr){
	framebuffer *fb = malloc(sizeof(struct framebuffer));
	FILE *infile = fopen(infilestr, "rb");
	int i, j;

	fb->w = 640;
	fb->h = 480;
	fb->mode = mode;
	switch(mode){
		// 320x240 native mode
		case 2:
			fb->w = 320;
			fb->h = 240;
			break;

		// 640x480 single-sample mode (very lossy)
		case 3:
			break;

		// 640x480 downsample mode
		case 1:
		default:
			fb->mode = 1;
	}
	fb->buffer = malloc(fb->w * fb->h * 2);
	fb->infile = infile;
	fb->flag = 1;

	for(i = 0; i < 240; i++){
		for(j = 0; j < 320; j++){
			fb->diffmap[i][j] = 1;
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
