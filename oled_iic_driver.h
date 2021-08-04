#ifndef __OLED_IIC_DRIVER_H__
#define __OLED_IIC_DRIVER_H__

#include <reg52.h>
#include <intrins.h>
#include "font_library.h"
sbit IIC_SCL = P3^7;
sbit IIC_SDA = P1^7;

#define IIC_SCL_SET IIC_SCL=1
#define IIC_SCL_CLR IIC_SCL=0
#define IIC_SDA_SET IIC_SDA=1
#define IIC_SDA_CLR IIC_SDA=0
#define X_WIDTH 128
#define Y_HIGHT 64


void iic_start();
void iic_stop();
bit  iic_write_byte(unsigned char dat);
void oled_write_cmd(unsigned char command);
void oled_write_dat(unsigned char dat);
void oled_set_position(unsigned char x, unsigned char y);
void oled_all_fill(unsigned char dat);
void oled_init(void);
void oled_ascii_6x8_str(unsigned char x, unsigned char y,unsigned char ch[]);
void oled_ascii_8x16_str(unsigned char x,unsigned char y,unsigned char ch[]);
//void oled_ascii_16x16_str(unsigned char x, y,unsigned char ch[]);
//void oled_ascii_16x24_str(unsigned char x, y,unsigned char ch[]);
void oled_chinese_characters_16x16(unsigned x,unsigned char y,unsigned char chinese_hex[]);
void oled_show_decimal_2_2(unsigned char x, unsigned char y,unsigned int dat);
void oled_show_decimal_3_1(unsigned char x, unsigned char y,unsigned int dat);

#endif