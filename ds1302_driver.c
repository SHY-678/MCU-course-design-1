#include "ds1302_driver.h"

unsigned char code ds1302_addr_tab[7] = {0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8c};
//ds1302的时间寄存器地址
unsigned char TIME[7] = {0x50, 0x59, 0x22, 0x31, 0x12, 0x02, 0x20};
//全局时间变量 秒 分 时 日 月 周 年 ，这里也是初始化时候给的时间初始值

/*******************************************************************************
* 函数功能		   : 向DS1302命令（地址+数据）
* 输    入         : addr,dat
* 输    出         : 无*/
void ds1302_write(unsigned char addr, unsigned char dat)
{
	unsigned char n;
	RST = 0;
	_nop_();

	SCLK = 0;		//先将SCLK置低电平。
	_nop_();
	RST = 1; 		//然后将RST(CE)置高电平。
	_nop_();

	for (n=0; n<8; n++)//开始传送八位地址命令
	{
		IO = addr & 0x01;//数据从低位开始传送
		addr >>= 1;
		SCLK = 1;	//数据在上升沿时，DS1302读取数据
		_nop_();
		SCLK = 0;
		_nop_();
	}
	for (n=0; n<8; n++)		//写入8位数据
	{
		IO = dat & 0x01;
		dat >>= 1;
		SCLK = 1;			//数据在上升沿时，DS1302读取数据
		_nop_();
		SCLK = 0;
		_nop_();	
	}	
		 
	RST = 0;		//传送数据结束
	_nop_();
}

/*******************************************************************************
* 函数功能		   : 读取一个地址的数据
* 输    入         : addr
* 输    出         : dat*/
unsigned char ds1302_read(unsigned char addr)
{
	unsigned char n,dat,dat1;
	addr |= 0x01;
	RST = 0;
	_nop_();

	SCLK = 0;		//先将SCLK置低电平。
	_nop_();
	RST = 1;		//然后将RST(CE)置高电平。
	_nop_();

	for(n=0; n<8; n++)//开始传送八位地址命令
	{
		IO = addr & 0x01;//数据从低位开始传送
		addr >>= 1;
		SCLK = 1;		//数据在上升沿时，DS1302读取数据
		_nop_();
		SCLK = 0;	//DS1302下降沿时，放置数据
		_nop_();
	}
	_nop_();
	for(n=0; n<8; n++)//读取8位数据
	{
		dat1 = IO;	//从最低位开始接收
		dat = (dat>>1) | (dat1<<7);
		SCLK = 1;
		_nop_();
		SCLK = 0;	//DS1302下降沿时，放置数据
		_nop_();
	}

	RST = 0;
	_nop_();		//以下为DS1302复位的稳定时间,必须的。
	SCLK = 1;
	_nop_();
	IO = 0;
	_nop_();
	IO = 1;
	_nop_();
	return dat;	
}

/*******************************************************************************
* 函数功能		   : 初始化DS1302.
* 输    入         : 无
* 输    出         : 无*/
void ds1302_init()
{
	unsigned char n;
	ds1302_write(0x8E,0x00);		 //禁止写保护，就是关闭写保护功能
	for (n=0; n<7; n++)		//写入7个字节的时钟信号：分秒时日月周年
	{
		ds1302_write(ds1302_addr_tab[n],TIME[n]);	
	}
	ds1302_write(0x8E,0x80);		 //打开写保护功能
}

/*******************************************************************************
* 函数功能		   : 读取时钟信息
* 输    入         : 无
* 输    出         : 无*/
void ds1302_read_all_time()
{
	unsigned char n;
	for (n=0; n<7; n++)	//读取7个字节的时钟信号：分秒时日月周年
	{
		TIME[n] = ds1302_read(ds1302_addr_tab[n]);
	}
		
}


