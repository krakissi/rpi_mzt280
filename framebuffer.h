typedef struct framebuffer {
	unsigned char *buffer;
	int xsize, ysize;
	FILE *infile;
	long fp;
	int drawmap[2][240][320];
	int diffmap[240][320];
	int flag;
	int scale;
} framebuffer;

framebuffer *framebuffer_create(int w, int h, const char *infilestr){
	framebuffer *fb = malloc(sizeof(struct framebuffer));
	FILE *infile = fopen(infilestr, "rb");
	int i, j;

	fb->xsize = w;
	fb->ysize = h;
	fb->infile = infile;
	fb->buffer = malloc(w * h * 2);
	fb->flag = 1;
	fb->scale = w / 320;

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
	fread(fb->buffer, fb->xsize * fb->ysize * 2, sizeof(unsigned char), fb->infile);
}
