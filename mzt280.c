#include "bcm2835.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/timeb.h>


#define RGB565(r, g, b) ((r >> 3) << 11 | (g >> 2) << 5 | ( b >> 3))
#define BCM2708SPI
#define ROTATE90

#define RGB565_MASK_RED   0xF800
#define RGB565_MASK_GREEN 0x07E0
#define RGB565_MASK_BLUE  0x001F

#ifdef ROTATE90
	#define XP 0x021
	#define YP 0x020

	#define XS 0x052
	#define XE 0x053
	#define YS 0x050
	#define YE 0x051

	#define MAX_X 319
	#define MAX_Y 239
#else
	#define XP 0x020
	#define YP 0x021

	#define XS 0x050
	#define XE 0x051
	#define YS 0x052
	#define YE 0x053

	#define MAX_X 239
	#define MAX_Y 319
#endif

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

int downscale(framebuffer *fb, int x, int y){
	int p, r, g, b, r1, g1, b1;
	unsigned long offset = 0;

	r = (p & RGB565_MASK_RED) >> 11;
	g = (p & RGB565_MASK_GREEN) >> 5;
	b = (p & RGB565_MASK_BLUE);

	r <<= 1;
	b <<= 1;

	offset = ((y + 1) * 320 * fb->scale + x) * 2;
	p = (fb->buffer[offset + 1] << 8) | fb->buffer[offset];
	r1 = (p & RGB565_MASK_RED) >> 11;
	g1 = (p & RGB565_MASK_GREEN) >> 5;
	b1 = (p & RGB565_MASK_BLUE);

	r += r1 << 1;
	g += g1;
	b += b1 << 1;

	offset = (y * 320 * fb->scale + x + 1) * 2;
	p= (fb->buffer[offset + 1] << 8) | fb->buffer[offset];
	r1 = (p & RGB565_MASK_RED) >> 11;
	g1 = (p & RGB565_MASK_GREEN) >> 5;
	b1 = (p & RGB565_MASK_BLUE);

	r += r1 << 1;
	g += g1;
	b += b1 << 1;

	offset = ((y + 1) * 320 * fb->scale + x + 1) * 2;
	p = (fb->buffer[offset + 1] << 8) | fb->buffer[offset];
	r1 = (p & RGB565_MASK_RED) >> 11;
	g1 = (p & RGB565_MASK_GREEN) >> 5;
	b1 = (p & RGB565_MASK_BLUE);

	r += r1 << 1;
	g += g1;
	b += b1 << 1;

	p = RGB565(r, g, b);

	return p;
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

		for(i = 0; i < 240; i++){
			for(j = 0; j < 320; j++){
				switch(fb->scale){
					case 2:
						p = downscale(fb, j << 1, i << 1);
						break;
					default:
						offset = (i * 320 * fb->scale + j) * 2;
						p = (fb->buffer[offset + 1] << 8) | fb->buffer[offset];
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

void LCD_Init(){
	LCD_RST_CLR;
	delay (100);
	LCD_RST_SET;
	delay (100);

	LCD_WR_CMD(0x0015, 0x0030); 	 // Set the internal vcore voltage
	LCD_WR_CMD(0x009A, 0x0010);

	//*************Power On sequence ****************
	LCD_WR_CMD(0x0011, 0x0020);		 // DC1[2:0], DC0[2:0], VC[2:0]
	LCD_WR_CMD(0x0010, 0x3428);		 // SAP, BT[3:0], AP, DSTB, SLP, STB
	LCD_WR_CMD(0x0012, 0x0002);		 // VREG1OUT voltage
	LCD_WR_CMD(0x0013, 0x1038);		 // VDV[4:0] for VCOM amplitude
	delay(4);
	LCD_WR_CMD(0x0012, 0x0012);		 // VREG1OUT voltage
	delay(4);
	LCD_WR_CMD(0x0010, 0x3420);		 // SAP, BT[3:0], AP, DSTB, SLP, STB
	LCD_WR_CMD(0x0013, 0x3038);		 // VDV[4:0] for VCOM amplitude
	delay(7);

	//----------2.8" Gamma Curve table 2 ----------
	LCD_WR_CMD(0x30, 0x0000);
	LCD_WR_CMD(0x31, 0x0402);
	LCD_WR_CMD(0x32, 0x0307);
	LCD_WR_CMD(0x33, 0x0304);
	LCD_WR_CMD(0x34, 0x0004);
	LCD_WR_CMD(0x35, 0x0401);
	LCD_WR_CMD(0x36, 0x0707);
	LCD_WR_CMD(0x37, 0x0305);
	LCD_WR_CMD(0x38, 0x0610);
	LCD_WR_CMD(0x39, 0x0610);

	LCD_WR_CMD(0x0001, 0x0000);	 // set SS and SM bit
	LCD_WR_CMD(0x0002, 0x0300);	 // set 1 line inveLCD_RSion
	LCD_WR_CMD(0x0003, 0x1038);	 // set GRAM write direction and BGR=1.
	LCD_WR_CMD(0x0008, 0x0808);	 // set the back porch and front porch
	LCD_WR_CMD(0x000A, 0x0008);	 // FMARK function

	LCD_WR_CMD(0x0060, 0x2700);		// Gate Scan Line
	LCD_WR_CMD(0x0061, 0x0001);		// NDL,VLE, REV

	LCD_WR_CMD(0x0090, 0x013E);
	LCD_WR_CMD(0x0092, 0x0100);
	LCD_WR_CMD(0x0093, 0x0100);

	LCD_WR_CMD(0x00A0, 0x3000);
	LCD_WR_CMD(0x00A3, 0x0010);

	//-------------- Panel Control -------------------//
	LCD_WR_CMD(0x0007, 0x0173);		// 262K color and display ON

	// FrameBuffer common setup code
	LCD_WR_CMD(XS, 0x0000); // Column address start
	LCD_WR_CMD(XE, MAX_X);  // Column address end
	LCD_WR_CMD(YS, 0x0000); // Row address start
	LCD_WR_CMD(YE, MAX_Y);  // Row address end

	LCD_WR_REG(0x22);
	LCD_CS_CLR;
	LCD_RS_SET;
}

void LCD_test(){
	int temp, num, i;
	char n;

	LCD_WR_CMD(XS, 0x0000); // Column address start2
	LCD_WR_CMD(XE, MAX_X); // Column address end2
	LCD_WR_CMD(YS, 0x0000); // Row address start2
	LCD_WR_CMD(YE, MAX_Y); // Row address end2

	LCD_WR_REG(0x22);
	LCD_CS_CLR;
	LCD_RS_SET;
	for(n = 0; n < 8; n++){
		temp = color[n];

		// FIXME: inline might break without block curlies.
		for(num = 40 * 240; num > 0; num--){
			LCD_WR_Data(temp);
		}
	}

	for(n = 0; n < 8; n++){
		LCD_WR_CMD(XS, 0x0000); // Column address start2
		LCD_WR_CMD(XE, MAX_X); // Column address end2
		LCD_WR_CMD(YS, 0x0000); // Row address start2
		LCD_WR_CMD(YE, MAX_Y); // Row address end2

		LCD_WR_REG(0x22);
		LCD_CS_CLR;
		LCD_RS_SET;
		temp = color[n];
		for(i = 0; i < 240; i++){
			// FIXME: inline might break without block curlies.
			for(num = 0; num < 320; num++){
		  		LCD_WR_Data(temp);
			}
		}
	}
	LCD_CS_SET;
}

/* Sets the state variable true, which should signal framebuffer loops to exit
 * when convenient. */
void gracefulexit(int na){
	state |= 0x1;
}

int main(int argc, char **argv){
	int c, state = 0;
	int displayMode = 0;
	int runTest = 0;

	while(!state && (c = getopt(argc, argv, "2t"))) switch(c){
		case '2':
			displayMode = 1;
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
		// FIXME useful message
		fprintf(stderr,
			"Usage:\t%s [flags]\n"
			"\t2 - 320x240 mode (default 640x480)\n"
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

	//bcm2835_spi_chipSelect(BCM2835_SPI_CS1);
	//bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS1,LOW);

	LCD_PWM_CLR;
	LCD_Init();

	if(runTest)
		LCD_test();

	signal(SIGUSR1, gracefulexit);
	loadFrameBuffer_diff((displayMode) ? framebuffer_create(320, 240, "/dev/fb0") : framebuffer_create(640, 480, "/dev/fb0"));

	return 0;
}
