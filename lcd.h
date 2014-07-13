void LCD_WR_REG(int index){
	LCD_CS_CLR;
	LCD_RS_CLR;

	bcm2835_spi_transfer(index >> 8);
	bcm2835_spi_transfer(index);

	LCD_CS_SET;
}

void LCD_WR_CMD(int index, int val){
	LCD_CS_CLR;
	LCD_RS_CLR;

	bcm2835_spi_transfer(index >> 8);
	bcm2835_spi_transfer(index);

	LCD_RS_SET;

	bcm2835_spi_transfer(val >> 8);
	bcm2835_spi_transfer(val);

	LCD_CS_SET;
}

void inline LCD_WR_Data(int val){
	//LCD_CS_CLR;
	//LCD_RS_SET;
	bcm2835_spi_transfer(val>>8);
	bcm2835_spi_transfer(val);
}

void write_dot(char dx, int dy, int color){
	//LCD_WR_CMD(XS,0x0000); // Column address start2
	//LCD_WR_CMD(XE,MAX_X); // Column address end2
	//LCD_WR_CMD(YS,0x0000); // Row address start2
	//LCD_WR_CMD(YE,MAX_Y); // Row address end2

	LCD_WR_CMD(XP,dy); // Column address start
	LCD_WR_CMD(YP,dx); // Row address start

	LCD_WR_CMD(0x22,color);
}


