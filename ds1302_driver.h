#ifndef __DS1302_DRIVER_H__
#define __DS1302_DRIVER_H__
#include<reg52.h>
#include<intrins.h>


sbit IO=P2^0;
sbit RST=P2^1;
sbit SCLK=P2^2;

extern unsigned char TIME[7];
void ds1302_write(unsigned char addr, unsigned char dat);
unsigned char ds1302_read(unsigned char addr);
void ds1302_init();
void ds1302_read_all_time();



#endif