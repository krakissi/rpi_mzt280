#include "bcm2835.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/timeb.h>


#define RGB565(r, g, b) ((r >> 3) << 11 | (g >> 2) << 5 | ( b >> 3))
#define BCM2708SPI

#define RGB565_MASK_RED   0xF800
#define RGB565_MASK_GREEN 0x07E0
#define RGB565_MASK_BLUE  0x001F

#define XP 0x021
#define YP 0x020

#define XS 0x052
#define XE 0x053
#define YS 0x050
#define YE 0x051

#define MAX_X 319
#define MAX_Y 239

#define	SPICS  RPI_GPIO_P1_24 //GPIO08
#define	SPIRS  RPI_GPIO_P1_22 //GPIO25
#define	SPIRST RPI_GPIO_P1_10 //GPIO15
#define	SPISCL RPI_GPIO_P1_23 //GPIO11
#define	SPISCI RPI_GPIO_P1_19 //GPIO10
#define	LCDPWM RPI_GPIO_P1_12 //GPIO18

#define LCD_CS_CLR  bcm2835_gpio_clr(SPICS)
#define LCD_RS_CLR  bcm2835_gpio_clr(SPIRS)
#define LCD_RST_CLR	bcm2835_gpio_clr(SPIRST)
#define LCD_SCL_CLR	bcm2835_gpio_clr(SPISCL)
#define LCD_SCI_CLR	bcm2835_gpio_clr(SPISCI)
#define LCD_PWM_CLR	bcm2835_gpio_clr(LCDPWM)

#define LCD_CS_SET  bcm2835_gpio_set(SPICS)
#define LCD_RS_SET  bcm2835_gpio_set(SPIRS)
#define LCD_RST_SET	bcm2835_gpio_set(SPIRST)
#define LCD_SCL_SET	bcm2835_gpio_set(SPISCL)
#define LCD_SCI_SET	bcm2835_gpio_set(SPISCI)
#define LCD_PWM_SET	bcm2835_gpio_set(LCDPWM)

short color[] = { 0xf800, 0x07e0, 0x001f, 0xffe0, 0x0000, 0xffff, 0x07ff, 0xf81f };

/* Global State for Graceful Exits */
char state = 0;

#include "lcd.h"
#include "framebuffer.h"

void setsample(int *r, int *g, int *b, int p){
	*r += (p & RGB565_MASK_RED) >> 11 << 1;
	*g += (p & RGB565_MASK_GREEN) >> 5;
	*b += (p & RGB565_MASK_BLUE) << 1;
}

int downscale(framebuffer *fb, int x, int y){
	int p, r = 0, g = 0, b = 0;
	unsigned long offset = 0;

	offset = (y * 640 + x) * 2;
	p = (fb->buffer[offset + 1] << 8) | fb->buffer[offset];
	setsample(&r, &g, &b, p);

	if(fb->mode == 3){
		// Ugly single-sample mode
		r <<= 2;
		g <<= 2;
		b <<= 2;
	} else {
		// Blended sample mode.
		// FIXME there has to be a more efficient way to get the offset
		offset = ((y + 1) * 640 + (x + 1)) * 2;
		p = (fb->buffer[offset + 1] << 8) | fb->buffer[offset];
		setsample(&r, &g, &b, p);

		offset = ((y + 1) * 640 + x) * 2;
		p = (fb->buffer[offset + 1] << 8) | fb->buffer[offset];
		setsample(&r, &g, &b, p);

		offset = (y * 640 + (x + 1)) * 2;
		p = (fb->buffer[offset + 1] << 8) | fb->buffer[offset];
		setsample(&r, &g, &b, p);
	}

	return RGB565(r, g, b);
}

void loadFrameBuffer_diff(framebuffer *fb){
	int i, j, p;
	int diffsx, diffsy, diffex, diffey;
	unsigned long offset = 0;
	int numdiff = 0;

	framebuffer_read(fb);

	while(!state){
		// Delay to reduce CPU saturation.
		usleep(17000);

		numdiff = 0;
		fb->flag = 1 - fb->flag;
		diffex = diffey = 0;
		diffsx = diffsy = 65535;

		for(i = 0; i <= MAX_Y; i++){
			for(j = 0; j <= MAX_X; j++){
				switch(fb->mode){
					case 2:
						offset = (i * (MAX_X + 1) + j) * 2;
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
					numdiff++;
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

		LCD_WR_CMD(YS, diffsx); // Column address start2
		LCD_WR_CMD(YE, diffex); // Column address end2
		LCD_WR_CMD(XS, diffsy); // Row address start2
		LCD_WR_CMD(XE, diffey); // Row address end2

		LCD_WR_CMD(XP, diffsy); // Column address start
		LCD_WR_CMD(YP, diffsx); // Row address start

		LCD_WR_REG(0x22);
		LCD_CS_CLR;
		LCD_RS_SET;

		for(i = diffsx; i <= diffex; i++){
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
