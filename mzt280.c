#include "bcm2835.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/timeb.h>
#include <time.h>

/* Global State for Graceful Exits */
char state = 0;

#include "lcd.h"
#include "framebuffer.h"

void setsample(int *r, int *g, int *b, framebuffer *fb, unsigned long offset){
	unsigned char *buf = &fb->buffer[offset];
	int p = (buf[1] << 8) | buf[0];

	*r += (p & RGB565_MASK_RED) >> 10;
	*g += (p & RGB565_MASK_GREEN) >> 5;
	*b += (p & RGB565_MASK_BLUE) << 1;
}

int downscale(framebuffer *fb, int x, int y){
	unsigned long offset = 0;
	int r = 0, g = 0, b = 0;

	offset = (y * 2 * DISP_W + x) * 2;
	setsample(&r, &g, &b, fb, offset);

	if(fb->mode == 3){
		// Ugly single-sample mode
		r <<= 2;
		g <<= 2;
		b <<= 2;
	} else {
		// Blended sample mode.
		offset += 2;
		setsample(&r, &g, &b, fb, offset);

		offset += 4 * DISP_W;
		setsample(&r, &g, &b, fb, offset);

		offset -= 2;
		setsample(&r, &g, &b, fb, offset);
	}

	return RGB565(r, g, b);
}

void loadFrameBuffer_diff(framebuffer *fb){
	int diffsx, diffsy, diffex, diffey;
	unsigned long offset = 0;
	long diff_count;
	int i, j, p;

	long dtime;
	struct timespec time_last, time_cur;
	time_last = (struct timespec){ 0, 0 };

	framebuffer_read(fb);

	while(!state){
		clock_gettime(CLOCK_REALTIME, &time_cur);
		if(time_cur.tv_sec - time_last.tv_sec <= 1){
			dtime = 100000L - ((time_cur.tv_sec - time_last.tv_sec) * 1e6L + (time_cur.tv_nsec - time_last.tv_nsec) / 1e3L);

			// Delay to save CPU
			if(dtime > 0)
				usleep(dtime);
		}
		time_last = time_cur;

		// Switch buffer planes
		fb->flag = 1 - fb->flag;

		diff_count = 0L;
		diffex = diffey = 0;
		diffsx = diffsy = 65535;

		for(i = 0; i <= MAX_Y; i++){
			for(j = 0; j <= MAX_X; j++){
				switch(fb->mode){
					case 2:
						offset = (i * DISP_W + j) * 2;
						p = (fb->buffer[offset + 1] << 8) | fb->buffer[offset];
						break;

					default:
						p = downscale(fb, j << 1, i << 1);
						break;
				}

				if(fb->drawmap[1 - fb->flag][i][j] != p) {
					fb->drawmap[fb->flag][i][j] = p;
					fb->diffmap[i][j] = 1;
					fb->drawmap[1 - fb->flag][i][j] = p;
					diff_count++;

					if(i < diffsx)
						diffsx = i;
					if(i > diffex)
						diffex = i; 
					if(j < diffsy)
						diffsy = j;
					if(j > diffey)
						diffey = j;
				} else fb->diffmap[i][j] = 0;
			}
		}

		if(diff_count < (DISP_W * DISP_H / 8)){
			// For small updates, writing individual dots looks better.
			for(i = 0; i <= MAX_Y; i++)
				for(j = 0; j <= MAX_X; j++)
					if(fb->diffmap[i][j])
						write_dot(i, j, fb->drawmap[fb->flag][i][j]);
		} else {
			// Large area writes are faster than single dots. Needed for big delta.
			LCD_WR_CMD(YS, diffsx); // Column address start
			LCD_WR_CMD(YE, diffex); // Column address end
			LCD_WR_CMD(XS, diffsy); // Row address start
			LCD_WR_CMD(XE, diffey); // Row address end

			LCD_WR_CMD(XP, diffsy); // Column address start
			LCD_WR_CMD(YP, diffsx); // Row address start

			LCD_WR_REG(0x22);
			LCD_CS_CLR;
			LCD_RS_SET;

			for(i = diffsx; i <= diffex; i++)
				for(j = diffsy; j <= diffey; j++)
					LCD_WR_Data(fb->drawmap[fb->flag][i][j]);
		}
		framebuffer_read(fb);
	}
}

/* Sets the state variable true, which should signal framebuffer loops to exit
 * when convenient. */
void gracefulexit(int na){
	state |= 0x1;
}

int main(int argc, char **argv){
	int displayMode = 0;
	int c, state = 0;
	int runTest = 0;

	while(!state && (c = getopt(argc, argv, "123t"))) switch(c){
		case '1':
		case '2':
		case '3':
			displayMode = c - '0';
			break;
		case 't':
			runTest = 1;
			break;

		case -1:
			state = 1;
			break;
		default:
			state = 2;
	}
	if(state == 2){
		fprintf(stderr,
			"Usage:\t%s [flags]\n"
			"\t1 - downsample 640x480 mode (default)\n"
			"\t2 - native 320x240 mode\n"
			"\t3 - ugly/fast downsample 640x480 mode\n"
			"\tt - Run display test at startup\n"
			"\n",
			*argv
		);

		return 0;
	}

	if(!bcm2835_init()){
		printf("bcm2835 init error\n");
		return 1;
	}

	bcm2835_gpio_fsel(SPICS, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(SPIRS, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(SPIRST, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(LCDPWM, BCM2835_GPIO_FSEL_OUTP);

	bcm2835_spi_begin();
	bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
	bcm2835_spi_setDataMode(BCM2835_SPI_MODE3);
	bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_2);

	LCD_PWM_CLR;
	LCD_Init();

	if(runTest)
		LCD_test();

	signal(SIGUSR1, gracefulexit);
	loadFrameBuffer_diff(framebuffer_create(displayMode, "/dev/fb0"));

	return 0;
}
