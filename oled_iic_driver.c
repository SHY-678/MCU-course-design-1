#include "oled_iic_driver.h"

/*******************************************************************************
* 函数功能		   : IIC Start
* 输    入         : 无
* 输    出         : 无*/
void iic_start()
{
    IIC_SCL_SET;       
    IIC_SDA_SET;
    _nop_();
    IIC_SDA_CLR;
    _nop_();
    IIC_SCL_CLR;

}

/*******************************************************************************
* 函数功能		   :IIC Stop
* 输    入         : 无
* 输    出         : 无*/
void iic_stop()
{
	IIC_SDA_CLR;
    _nop_();
    IIC_SCL_SET;
    _nop_();
    IIC_SDA_SET;
}

/*******************************************************************************
* 函数功能		   :IIC Write byte
* 输    入         :要写入的数据
* 输    出         : 应答状态*/
bit iic_write_byte(unsigned char dat)
{
	unsigned char i;
    bit ack_bit;                    
    for(i=0;i<8;i++)               
    {
		if(dat & 0x80)//b1000 0000          
            IIC_SDA_SET;
        else
            IIC_SDA_CLR;
        IIC_SCL_SET;
        _nop_();_nop_();
        IIC_SCL_CLR;
        dat<<=1;
    }
    IIC_SDA_SET;                          
    _nop_();_nop_();
    IIC_SCL_SET;                    
    _nop_();_nop_();
    ack_bit = IIC_SDA;//acknowledeg status                          
    IIC_SCL_CLR;
    return ack_bit;       
}  

/*******************************************************************************
* 函数功能		   :oled Write Command
* 输    入         :要写入的一个命令
* 输    出         : 无*/
void oled_write_cmd(unsigned char command)
{
   iic_start();
   iic_write_byte(0x78);            //Slave address,SA0=0
   iic_write_byte(0x00);            //write command b00 000000
   iic_write_byte(command);
   iic_stop();
}

/*******************************************************************************
* 函数功能		   :oled Write Data
* 输    入         :要写入的一个数据
* 输    出         : 无*/
void oled_write_dat(unsigned char dat)
{
   iic_start();
   iic_write_byte(0x78);                       
   iic_write_byte(0x40);            //write data b01 000000
   iic_write_byte(dat);
   iic_stop();
}

/*******************************************************************************
* 函数功能		   :oled set position
* 输    入         :列起始地址0-127，页起始地址0-7
* 输    出         : 无*/
void oled_set_position(unsigned char x, unsigned char y)
{
    oled_write_cmd(0xb0+y);            //Set Page Start Address for Page Addressing Mode
    oled_write_cmd(((x&0xf0)>>4)|0x10);//Set Higher Column 
									   //Start Address for Page Addressing Mode
    oled_write_cmd((x&0x0f));		   //Set Lower Column 
									   //Start Address for Page Addressing Mode
}

/*******************************************************************************
* 函数功能		   :oled所有点写1或0
* 输    入         :写入的状态0x00 or 0xff
* 输    出         : 无*/
void oled_all_fill(unsigned char status)
{
    unsigned char y,x;
    for(y=0;y<8;y++)
    {
        oled_write_cmd(0xb0+y);
        oled_write_cmd(0x00);
        oled_write_cmd(0x10);
        for(x=0;x<X_WIDTH;x++)
			oled_write_dat(status);
    }
}

/*******************************************************************************
* 函数功能		   :oled initialization
* 输    入         :无
* 输    出         : 无*/
void oled_init(void)     
{
    unsigned char tc = 0;
	while(--tc);
    oled_write_cmd(0xae);//--turn off oled panel
    oled_write_cmd(0x00);//---set low column address
    oled_write_cmd(0x10);//---set high column address
    oled_write_cmd(0x40);//--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
    oled_write_cmd(0x81);//--set contrast control register
    oled_write_cmd(0xcf); // Set SEG Output Current Brightness
    oled_write_cmd(0xa1);//--Set SEG/Column Mapping     
    oled_write_cmd(0xc8);//Set COM/Row Scan Direction   
    oled_write_cmd(0xa6);//--set normal display
    oled_write_cmd(0xa8);//--set multiplex ratio(1 to 64)
    oled_write_cmd(0x3f);//--1/64 duty
    oled_write_cmd(0xd3);//-set display offset        Shift Mapping RAM Counter (0x00~0x3F)
    oled_write_cmd(0x00);//-not offset
    oled_write_cmd(0xd5);//--set display clock divide ratio/oscillator frequency
    oled_write_cmd(0x80);//--set divide ratio, Set Clock as 100 Frames/Sec
    oled_write_cmd(0xd9);//--set pre-charge period
    oled_write_cmd(0xf1);//Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
    oled_write_cmd(0xda);//--set com pins hardware configuration
    oled_write_cmd(0x12);
    oled_write_cmd(0xdb);//--set vcomh
    oled_write_cmd(0x40);//Set VCOM Deselect Level
    oled_write_cmd(0x20);//-Set Page Addressing Mode (0x00/0x01/0x02)
    oled_write_cmd(0x02);//
    oled_write_cmd(0x8d);//--set Charge Pump enable/disable
    oled_write_cmd(0x14);//--set(0x10) disable
    oled_write_cmd(0xa4);// Disable Entire Display On (0xa4/0xa5)
    oled_write_cmd(0xa6);// Disable Inverse Display On (0xa6/a7)
    oled_write_cmd(0xaf);//--turn on oled panel
    oled_all_fill(0x00);  //
    oled_set_position(0,0);        
}
///***************显示6*8一组标准ASCII字符串 坐标(x,y),y为页范围0~7****************/
//void oled_ascii_6x8_str(unsigned char x,unsigned char y,unsigned char ch[])
//{
//    unsigned char c=0,i=0,j=0;
//    while (ch[j]!='\0')
//    {
//        c =ch[j]-32;
//        if(x>127)
//		{
//			x=0;
//			y++;
//		}
//        oled_set_position(x,y);
//        for(i=0;i<6;i++)
//            oled_write_dat(F8x6[c][i]);
//        x+=6;
//        j++;
//    }
//}
/*******************显示8*16一组标准ASCII字符串 坐标(x,y),y为页范围0~7****************/
void oled_ascii_8x16_str(unsigned char x,unsigned char y,unsigned char ch[])
{
    unsigned char c=0,i=0,j=0;
    while (ch[j]!='\0')
    {
        c =ch[j]-32;
        if(x>127)
		{
			x=0;
			y+=2;
		}
        oled_set_position(x,y);
        for(i=0;i<8;i++)
            oled_write_dat(F16X8[c][i]);
        oled_set_position(x,y+1);
        for(i=0;i<8;i++)
            oled_write_dat(F16X8[c][i+8]);
        x+=8;
        j++;
    }
}
///******************显示16*16一组标准ASCII字符串 坐标(x,y),y为页范围**************************************************/

//void oled_ascii_16x16_str(unsigned char x, y,unsigned char ch[])
//{
//    unsigned char c=0,i=0,j=0;
//    while (ch[j]!='\0')
//    {
//        c =ch[j]-32;
//        if(x>127)
//		{
//			x=0;
//			y++;
//		}
//        oled_set_position(x,y);
//        for(i=0;i<16;i++)
//            oled_write_dat(F16X16[c*32+i]);//库没有定义
//        oled_set_position(x,y+1);
//        for(i=0;i<16;i++)
//            oled_write_dat(F16X16[c*32+i+16]);//库没有定义
//        x+=16;
//        j++;
//    }
//}
///******************显示16*24一组标准ASCII字符串 坐标(x,y),y为页范围0~7**************************************************/

//void oled_ascii_16x24_str(unsigned char x, y,unsigned char ch[])
//{
//    unsigned char c=0,i=0,j=0;
//    while (ch[j]!='\0')
//    {
//        c =ch[j]-32;
//        if(x>112){x=0;y++;}
//            oled_set_position(x,y);
//        for(i=0;i<16;i++)
//            oled_write_dat(F16X24[c*48+i]);//库没有定义
//        oled_set_position(x,y+1);
//        for(i=0;i<16;i++)
//            oled_write_dat(F16X24[c*48+i+16]);//库没有定义
//        oled_set_position(x,y+2);
//        for(i=0;i<16;i++)
//            oled_write_dat(F16X24[c*48+i+16*2]);//库没有定义
//        x+=16;
//        j++;
//    }
//}

/***********************************************************************************/
void oled_chinese_characters_16x16(unsigned x,unsigned char y,unsigned char chinese_hex[32])
{
	unsigned char i=0,j=0;
//    if(x<=127&&y<=7)
	oled_set_position(x,y);
    for(i=0;i<16;i++)
        oled_write_dat(chinese_hex[i]);
    oled_set_position(x,y+1);
    for(i=0;i<16;i++)
        oled_write_dat(chinese_hex[i+16]);
}
void oled_show_decimal_2_2(unsigned char x,unsigned char y,unsigned int dat)
{
    unsigned char translate_data[6];
    translate_data[0] = dat % 10000 / 1000 + '0';
    translate_data[1] = dat % 1000 / 100 + '0';
    translate_data[2] = '.';
    translate_data[3] = dat % 100 / 10 + '0';
    translate_data[4] = dat % 10 + '0';
    translate_data[5] = '\0';
    oled_ascii_8x16_str(x,y,translate_data) ;
}

/***********************************************************************************/
void oled_show_decimal_3_1(unsigned char x,unsigned char y,unsigned int dat)
{

    unsigned char translate_data[6];
    translate_data[0] = dat % 100000/ 10000 + '0' ;
    translate_data[1] = dat % 10000 / 1000 + '0';
    translate_data[2] = dat % 1000 / 100 + '0';
    translate_data[3] = '.';
    translate_data[4] = dat % 100 / 10 + '0';
    // translate_date[5] = dat % 10 + '0';
    translate_data[5] = '\0';
    oled_ascii_8x16_str(x,y,translate_data) ;
    //oled_ascii_16x24_str(x,y,translate_data) ;
}

/*****************显示16*16点阵  (x,y),y0~7****************************/
//void LCD_P16x16Ch(unsigned char x, y, N)
//{
//        unsigned char wm=0;
//        unsigned int adder=32*N;
//        LCD_Set_Pos(x , y);
//        for(wm = 0;wm < 16;wm++)
//        {
//                LCD_WrDat(F16x16[adder]);
//                adder += 1;
//        }
//        LCD_Set_Pos(x,y + 1);
//        for(wm = 0;wm < 16;wm++)
//        {
//                LCD_WrDat(F16x16[adder]);
//                adder += 1;
//        }                  
//}
/***********显示BMP128*64图片 起始坐标(x,y),x0~127,y页0~7*****************/
//void Draw_BMP(unsigned char x0, y0,x1, y1,unsigned char BMP[])
//{        
// unsigned int j=0;
// unsigned char x,y;
//  
//  if(y1%8==0) y=y1/8;      
//  else y=y1/8+1;
//        for(y=y0;y<y1;y++)
//        {
//                LCD_Set_Pos(x0,y);
//    for(x=x0;x<x1;x++)
//            {      
//                    LCD_WrDat(BMP[j++]);                   
//            }
//        }
//}


