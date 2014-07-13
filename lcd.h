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

		for(num = 40 * 240; num > 0; num--)
			LCD_WR_Data(temp);
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

		for(i = 0; i < 240; i++)
			for(num = 0; num < 320; num++)
		  		LCD_WR_Data(temp);
	}
	LCD_CS_SET;
}
