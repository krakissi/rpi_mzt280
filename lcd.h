/*
	lcd
*/
#ifndef MZT280_LCD_H
#define MZT280_LCD_H

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
#define DISP_W (MAX_X + 1)
#define DISP_H (MAX_Y + 1)

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

extern void LCD_WR_REG(int index);
extern void LCD_WR_CMD(int index, int val);
extern void inline LCD_WR_Data(int val);
extern void write_dot(char dx, int dy, int color);

extern void LCD_Init();
extern void LCD_test();

#endif
