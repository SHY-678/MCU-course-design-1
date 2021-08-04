#include <reg52.h>
#include "ds1302_driver.h"
#include "oled_iic_driver.h"

/*程序运行状态标志*/
#define show_state 0		//显示模式
#define menu_state  1        //菜单模式
#define set_alarm  2        //闹钟设置
#define set_time   3   		//时间设置
#define alarm_noisy 4

//按键按下的电平状态
#define key_down 0			
#define key_up   1

sbit menu_key = P2^7;//将按键跳到P3^2 外部中断0输入
sbit shift_key = P2^6;
sbit add_key = P2^5;
sbit save_key = P2^4;
sbit alarm = P3^4;

unsigned char temp_buf[10];		//缓冲区
unsigned char alarm_time[2][3] ={{0x00, 0x30,0x08},{0x00, 0x20,0x08}};//闹钟时间2组开关分时bcd码存放
unsigned char *time_ascii;				//用于存放转换成ascii后的时间数据
unsigned char run_state = show_state;	//初始化程序运行状态为显示模式
unsigned char timer0_count = 1;			//定时器溢出次数
unsigned char key_pressed = 0;			//有无按键按下
unsigned char tc = 0;					//用于消抖
unsigned char my_8bit_flag = 0x00;		//8位标志位
										//显示标志 菜单标志 设闹钟标志 设时间标志 
										//闪烁标志 alarm 选中time set  选中alarm set
int position = 0;//设置时间时候的光标位置
unsigned char blink_count = 0;

//***********************************************************************************
void alarm_show(void);

void delay1ms(void);
	
unsigned char* ds1302_time_dat_to_ascii(unsigned char time[]);/* *将ds1302读出的数据转换为ascii码*/	

void show_time(unsigned char ascii_time[],unsigned char page_drift);/* *用于显示时间*/

void bcd_add_1(unsigned char *bcd_dat,unsigned char position);//设置时间或闹钟时调用，对应位置的bcd码加一

void Timer0Init(void);//50毫秒定时器0初始化@12.000MHz		


void main(void)
{
	
	P3 |= 0x10;//屏蔽烦躁人的蜂鸣器
	//ds1302_init();
	oled_init();
	Timer0Init();
	IT0=0; //外部中断0低电平有效
	EX0=1; //外部中断开启
	ET0=1; //开启定时器0中断
	TR0 = 0;
	EA=1;  //总中断开启
	while(1)
	{
		switch(run_state)
		{
			case show_state:
				if((my_8bit_flag&0x80)==0x00)//刚进来时 清一次屏
				{	
					oled_all_fill(0x00);
					my_8bit_flag |= 0x80;
					oled_ascii_8x16_str(0,6,"Alarm\0");
					if(alarm_time[0][0])
						oled_ascii_8x16_str(42,6,"1 ON\0");
					else
						oled_ascii_8x16_str(42,6,"1 OFF\0");
					if(alarm_time[1][0])
						oled_ascii_8x16_str(85,6,"2 ON\0");
					else
						oled_ascii_8x16_str(85,6,"2 OFF\0");
						
				}
				ds1302_read_all_time();
				time_ascii = ds1302_time_dat_to_ascii(TIME);
				show_time(time_ascii,0);
				if(alarm_time[0][0])
					if((TIME[1]==alarm_time[0][1]) && (TIME[2]==alarm_time[0][2]) && (TIME[0]==0x00))
						run_state = alarm_noisy;
				if(alarm_time[1][0])
					if((TIME[1]==alarm_time[1][1]) && (TIME[2]==alarm_time[1][2]) && (TIME[0]==0x00))
						run_state = alarm_noisy;
				break;
			case alarm_noisy:
				alarm = ~alarm;
				timer0_count = 0;
				{
					unsigned char tc = 200;
					while(tc--)
						delay1ms();
				}
				if((shift_key==key_down)||(add_key==key_down)||(save_key==key_down))
				{
					unsigned char tc = 80;
					key_pressed = 1;
					while(!(tc--)) delay1ms();
						
					if((shift_key==key_down)||(add_key==key_down)||(save_key==key_down))
					{	
						alarm = 1;
						run_state = show_state;
					}

				}
				break;
			case menu_state:
				if((my_8bit_flag&0x40)==0x00)//刚进来时 清一次屏只显示一次菜单，避免循环显示对程序带来较大延时
				{	
					oled_all_fill(0x00);
					oled_ascii_8x16_str(12,0,"<Option Menu>\0");
					oled_ascii_8x16_str(2,2,"1.Time Set\0");
					oled_ascii_8x16_str(2,4,"2.Alarm Set\0");
					oled_ascii_8x16_str(0,6,"@Author:CTBU^Shy\0");
					my_8bit_flag |= 0x40;
				}
				/* *按键检测 */
				if((key_pressed==0)&&((shift_key==key_down)||(add_key==key_down)||(save_key==key_down)))
				{
					
					tc = 20;//用于消抖
					key_pressed = 1;
				}
				delay1ms();
				if(key_pressed)
				{
					tc--;
					if(!tc)//消抖结束
					{
						if(add_key == key_down)//add_key按下
						{
							my_8bit_flag |= 0x01;//alarm set
							my_8bit_flag &= 0xfd;//off time set
							timer0_count = 0;
							key_pressed = 0;
							oled_ascii_8x16_str(80,2,"    \0");
							oled_ascii_8x16_str(88,4,"<-<-\0");
						}
						else if(shift_key == key_down)//shift_key按下
						{
							my_8bit_flag |= 0x02;//time set
							my_8bit_flag &= 0xfe;//off alram set
							timer0_count = 0;
							key_pressed = 0;
							oled_ascii_8x16_str(80,2,"<-<-\0");
							oled_ascii_8x16_str(88,4,"    \0");
						}
						else if(save_key == key_down)//save_key按下
						{
							if((my_8bit_flag&0x01) == 0x01)//alarm set
							{
								timer0_count = 0;
								TR0 = 1;
								EX0 = 0;
								my_8bit_flag |= 0x03;//退出时清除选项按键缓存
								run_state = set_alarm;
								key_pressed = 0;
								break;//退出菜单
							}
							else if((my_8bit_flag&0x02) == 0x02)//time set
							{
								timer0_count = 0;
								TR0 = 1;
								EX0 = 0;
								my_8bit_flag |= 0x03;//退出时清除选项按键缓存
								run_state = set_time;
								key_pressed = 0;	
								break;//退出菜单
								
							}
							else
								key_pressed = 0;
						}
							
						else
							key_pressed = 0;//判定为抖动
							
					}
				}
				break;
			case set_alarm:
				if((my_8bit_flag&0x20)==0x00)//只显示一次菜单，避免循环显示对程序带来较大延时
				{
					oled_all_fill(0x00);
					oled_ascii_8x16_str(12,0,"< Alarm Set >\0");
					oled_ascii_8x16_str(2,2,"Alarm1@\0");
					oled_ascii_8x16_str(2,4,"Alarm2@\0");
					alarm_show();
					my_8bit_flag |= 0x20;
					position = 0;
					
					TIME[0] = alarm_time[0][0];
					TIME[1] = alarm_time[0][1];
					TIME[2] = alarm_time[0][2];
					TIME[3] = alarm_time[1][0];
					TIME[4] = alarm_time[1][1];
					TIME[5] = alarm_time[1][2];
				}
				/*闪烁设置闹钟的实现*///12.16
				if(my_8bit_flag&0x08)
				{
					if(--blink_count==0)
						my_8bit_flag&=0xf7;
				}
				else
				{
					if(++blink_count==10)
						my_8bit_flag|=0x08;
				}
				switch(position)
				{
					case 0:
						temp_buf[0]=(alarm_time[1][2]&0x30)/16 + '0';
						temp_buf[1]=(alarm_time[1][2]&0x0f) + '0';    //时
						temp_buf[2]='\0';
						oled_ascii_8x16_str(58,4,temp_buf);//点亮前一位
						
						if(my_8bit_flag&0x08)
						{
							if(alarm_time[0][0] == 0x00)
								oled_ascii_8x16_str(104,2,"OFF");
							else
								oled_ascii_8x16_str(104,2," ON");
						}
						else
						{
								oled_ascii_8x16_str(104,2,"   ");
						}
						break;
					case 1:
						if(alarm_time[0][0] == 0x00)
							oled_ascii_8x16_str(104,2,"OFF");
						else
							oled_ascii_8x16_str(104,2," ON");
						if(my_8bit_flag&0x08)
						{
							temp_buf[3]=(alarm_time[0][1]&0x70)/16 + '0';
							temp_buf[4]=(alarm_time[0][1]&0x0f) + '0';//分
							temp_buf[5]='\0';
							oled_ascii_8x16_str(58+24,2,&temp_buf[3]);
						}
						else
						{
							oled_ascii_8x16_str(58+24,2,"  ");
						}
						
						break;
					case 2:
						temp_buf[3]=(alarm_time[0][1]&0x70)/16 + '0';
						temp_buf[4]=(alarm_time[0][1]&0x0f) + '0';//分
						temp_buf[5]='\0';
						oled_ascii_8x16_str(58+24,2,&temp_buf[3]);
						if(my_8bit_flag&0x08)
						{
							temp_buf[0]=(alarm_time[0][2]&0x30)/16 + '0';
							temp_buf[1]=(alarm_time[0][2]&0x0f) + '0';    //时
							temp_buf[2]='\0';
							oled_ascii_8x16_str(58,2,temp_buf);
						}
						else
						{
							oled_ascii_8x16_str(58,2,"  ");
						}
						break;
					
					
					case 3:
						temp_buf[0]=(alarm_time[0][2]&0x30)/16 + '0';
						temp_buf[1]=(alarm_time[0][2]&0x0f) + '0';    //时
						temp_buf[2]='\0';
						oled_ascii_8x16_str(58,2,temp_buf);
						if(my_8bit_flag&0x08)
						{
							if(alarm_time[1][0] == 0x00)
								oled_ascii_8x16_str(104,4,"OFF");
							else
								oled_ascii_8x16_str(104,4," ON");
						}
						else
						{
								oled_ascii_8x16_str(104,4,"   ");
						}
						break;
					case 4:
						if(alarm_time[1][0] == 0x00)
								oled_ascii_8x16_str(104,4,"OFF");
						else
								oled_ascii_8x16_str(104,4," ON");
						if(my_8bit_flag&0x08)
						{
							temp_buf[3]=(alarm_time[1][1]&0x70)/16 + '0';
							temp_buf[4]=(alarm_time[1][1]&0x0f) + '0';//分
							temp_buf[5]='\0';
							oled_ascii_8x16_str(58+24,4,&temp_buf[3]);
						}
						else
						{
							oled_ascii_8x16_str(58+24,4,"  ");
						}
						break;
					case 5:
						temp_buf[3]=(alarm_time[1][1]&0x70)/16 + '0';
						temp_buf[4]=(alarm_time[1][1]&0x0f) + '0';//分
						temp_buf[5]='\0';
						oled_ascii_8x16_str(58+24,4,&temp_buf[3]);
						if(my_8bit_flag&0x08)
						{
							temp_buf[0]=(alarm_time[1][2]&0x30)/16 + '0';
							temp_buf[1]=(alarm_time[1][2]&0x0f) + '0';    //时
							temp_buf[2]='\0';
							oled_ascii_8x16_str(58,4,temp_buf);
						}
						else
						{
							oled_ascii_8x16_str(58,4,"  ");
						}
						break;
					default:
						break;
				}
				/* *按键检测 */
				if((key_pressed==0)&&((shift_key==key_down)||(add_key==key_down)||(save_key==key_down)))
				{
					tc=2;
					key_pressed = 1;
				}
				delay1ms();
				if(key_pressed&&!(tc--))
				{
					if(add_key == key_down)//add_key按下
					{
						timer0_count = 0;
						key_pressed = 0;//开放按键读入
						if(position < 3)
						{
							if(position == 0)
								alarm_time[0][0] = ~alarm_time[0][0];//开关状态改变
							else
								bcd_add_1(alarm_time[0],position);//时间设置
						}
						else
						{
							if(position == 3)
								alarm_time[1][0] = ~alarm_time[1][0];//开关状态改变
							else
								bcd_add_1(alarm_time[1],position-3);//时间设置
						}
						alarm_show();//刷新显示闹钟	
					}
					else if(shift_key == key_down)//shift_key按下
					{
						position++;
						if(position>5)//6个设置位
							position = 0;
						
						timer0_count = 0;
						
						key_pressed = 0;//开放按键读取
					}
					else if(save_key == key_down)//save_key按下
					{
						key_pressed = 0;//开放按键读取
						my_8bit_flag&=0xf7;
						run_state = show_state;//返回显示时间模式
					}
							
					else
						key_pressed = 0;//判定为抖动
				}
				break;
			case set_time:
				/*闪烁设置时间的实现*///12.16
				if(my_8bit_flag&0x08)
				{
					if(--blink_count==0)
						my_8bit_flag&=0xf7;
				}
				else
				{
					if(++blink_count==10)
						my_8bit_flag|=0x08;
				}
				switch(position)
				{
					unsigned char temp[3];
					case 6:
						{//显示前一位闪烁的值
						temp[0] = time_ascii[18];
						temp[1] = time_ascii[19];	
						temp[2] = '\0';
						oled_ascii_8x16_str(31+24+24,5,temp);//显示miao
						}
						
						if(my_8bit_flag&0x08)
						{

							temp[0] = time_ascii[2];
							temp[1] = time_ascii[3];	
							temp[2] = '\0';
							oled_ascii_8x16_str(16,2,temp);//显示年
						}
						else
						{
							oled_ascii_8x16_str(16,2,"  ");
						}
						break;
					case 5:
						{
							temp[0] = time_ascii[2];
							temp[1] = time_ascii[3];	
							temp[2] = '\0';
							oled_ascii_8x16_str(16,2,temp);//显示年
						}
						if(my_8bit_flag&0x08)
						{
							oled_chinese_characters_16x16(10*8+16*2,2,WEEK[time_ascii[11]-'0'+1]);//几
						}
						else
						{
							oled_ascii_8x16_str(10*8+16*2,2,"  ");
						}
						break;
					case 4:
						{
							oled_chinese_characters_16x16(10*8+16*2,2,WEEK[time_ascii[11]-'0'+1]);//几
						}
						if(my_8bit_flag&0x08)
						{
							temp[0] = time_ascii[5];
							temp[1] = time_ascii[6];	
							temp[2] = '\0';
							oled_ascii_8x16_str(16+24,2,temp);//显示月
						}
						else
						{
							oled_ascii_8x16_str(16+24,2,"  ");
						}
						
						break;
					case 3:
						{
							temp[0] = time_ascii[5];
							temp[1] = time_ascii[6];	
							temp[2] = '\0';
							oled_ascii_8x16_str(16+24,2,temp);//显示月
						}
						if(my_8bit_flag&0x08)
						{
							temp[0] = time_ascii[8];
							temp[1] = time_ascii[9];	
							temp[2] = '\0';
							oled_ascii_8x16_str(16+24+24,2,temp);//显示日
						}
						else
						{
							oled_ascii_8x16_str(16+24+24,2,"  ");
						}
						break;
					case 2:
						{
							temp[0] = time_ascii[8];
							temp[1] = time_ascii[9];	
							temp[2] = '\0';
							oled_ascii_8x16_str(16+24+24,2,temp);//显示日
						}
						if(my_8bit_flag&0x08)
						{
							temp[0] = time_ascii[12];
							temp[1] = time_ascii[13];	
							temp[2] = '\0';
							oled_ascii_8x16_str(31,5,temp);//显示时
						}
						else
						{
							oled_ascii_8x16_str(31,5,"  ");
						}
						break;
					case 1:
						{
							temp[0] = time_ascii[12];
							temp[1] = time_ascii[13];	
							temp[2] = '\0';
							oled_ascii_8x16_str(31,5,temp);//显示时
						}
						if(my_8bit_flag&0x08)
						{
							temp[0] = time_ascii[15];
							temp[1] = time_ascii[16];	
							temp[2] = '\0';
							oled_ascii_8x16_str(31+24,5,temp);//显示分
						}
						else
						{
							oled_ascii_8x16_str(31+24,5,"  ");//显示分
						}			
						break;
					case 0:
						{
							temp[0] = time_ascii[15];
							temp[1] = time_ascii[16];	
							temp[2] = '\0';
							oled_ascii_8x16_str(31+24,5,temp);//显示分
						}
						if(my_8bit_flag&0x08)
						{
							temp[0] = time_ascii[18];
							temp[1] = time_ascii[19];	
							temp[2] = '\0';
							oled_ascii_8x16_str(31+24+24,5,temp);//显示miao
						}
						else
						{
							oled_ascii_8x16_str(31+24+24,5,"  ");//显示miao
						}			
						break;	
					default:
						break;
				}
				
				if((my_8bit_flag&0x10)==0x00)//只显示一次菜单，避免循环显示对程序带来较大延时
				{
					oled_all_fill(0x00);
					oled_ascii_8x16_str(12,0,"< Time  Set >\0");
					my_8bit_flag |= 0x10;
					ds1302_read_all_time();//读一次时间到time全局变量
					time_ascii = ds1302_time_dat_to_ascii(TIME);
					show_time(time_ascii,2);//显示一次当前的时间
					position = 6;//初始化设置年位置
				}
				/* *按键检测 */
				if((key_pressed==0)&&((shift_key==key_down)||(add_key==key_down)||(save_key==key_down)))
				{
					tc=2;
					key_pressed = 1;
				}
				delay1ms();
				if(key_pressed&&!(tc--))
				{
					if(add_key == key_down)//add_key按下
					{
						timer0_count = 0;
						bcd_add_1(TIME,position);
						key_pressed = 0;//开放按键读入
						time_ascii = ds1302_time_dat_to_ascii(TIME);//刷新显示
						//show_time(time_ascii,2);
					}
					else if(shift_key == key_down)//shift_key按下
					{
						
						position--;
						if(position==-1)//7个设置位
							position = 6;
						timer0_count = 0;
						
						key_pressed = 0;//开放按键读取
					}
					else if(save_key == key_down)//save_key按下
					{
						ds1302_init();//重新写入新设置的时间到1302
						key_pressed = 0;//开放按键读取
						position=0;
						my_8bit_flag&=0xf7;
						run_state = show_state;//返回显示时间模式
					}	
					else
						key_pressed = 0;//判定为抖动
				}
				
				break;
			default:
				break;
				
		}
		
	}
}

unsigned char* ds1302_time_dat_to_ascii(unsigned char time[])
{
	static unsigned char time_ascii[] ="2020-11-09-120:00:12\0";
	//存放转换后的ascii码年-月-日-周时：分：秒
	unsigned char* TIME = time;
	time_ascii[19] = (TIME[0]&0x0f) + '0';		//秒
	time_ascii[18] = (TIME[0]&0x70)/16 + '0';
	
	time_ascii[16] = (TIME[1]&0x0f) + '0';		//分
	time_ascii[15] = (TIME[1]&0x70)/16 + '0';
		
	time_ascii[13] = (TIME[2]&0x0f) + '0';		//时
	time_ascii[12] = (TIME[2]&0x30)/16 + '0';
		
	time_ascii[11] = (TIME[5]&0x07) + '1';		//周
		
	time_ascii[9] = (TIME[3]&0x0f) + '0';		//日
	time_ascii[8] = (TIME[3]&0x30)/16 + '0';
		
	time_ascii[6] = (TIME[4]&0x0f) + '0';		//月
	time_ascii[5] = (TIME[4]&0x10)/16 + '0';
		
	time_ascii[3] = (TIME[6]&0x0f) + '0';		//年
	time_ascii[2] = (TIME[6]&0xf0)/16 + '0';
	
	time_ascii[1] =  '0';
	time_ascii[0] =  '2';
	return time_ascii;
}
void show_time(unsigned char ascii_time[],unsigned char page_drift)
{


	unsigned char* temp = ascii_time;
	temp[10] = '\0';//取年月日
	oled_ascii_8x16_str(0,page_drift,temp);//显示年月日
	oled_chinese_characters_16x16(10*8,page_drift,WEEK[0]);//星
	oled_chinese_characters_16x16(10*8+16,page_drift,WEEK[1]);//期
	oled_chinese_characters_16x16(10*8+16*2,page_drift,WEEK[ascii_time[11]-'0'+1]);//几
	temp = ascii_time;
	temp = temp+12;//取时时分秒
	oled_ascii_8x16_str(31,3+page_drift,temp);//显示时分秒
}

void Timer0Init(void)		//50毫秒@12.000MHz
{
	TMOD = 0x01;		//设置定时器为16位
	TL0 = 0xB0;		//设置定时器初值
	TH0 = 0x3C;		//设置定时器初值
	TF0 = 0;		//清除中断标志位
	TR0 = 0;		//关闭定时器
}
void exint0(void) interrupt 0		//外部中断0的中断服务函数
{
	TR0 = 1;//打开定时器
	EX0 = 0;
}

void timer0(void) interrupt 1		//定时器0的中断服务函数
{
	TL0 = 0xB0;		//设置定时器初值
	TH0 = 0x3C;		//设置定时器初值
	if(menu_key == key_down)//设置按键的消抖
	{
		
		if(run_state != menu_state)
			my_8bit_flag &= 0x0f;//高4位标志清零
		run_state = menu_state;
	}
	else
		EX0 = 1;
	if(run_state != show_state)
		timer0_count++;
	if(timer0_count >200)//如果10秒没有设置操作，退出菜单
	{
		
		if(run_state == set_alarm)//从设置闹钟退回来
		{
			alarm_time[0][0] = TIME[0];//不保存设置
			alarm_time[0][1] = TIME[1];
			alarm_time[0][2] = TIME[2];
			alarm_time[1][0] = TIME[3];
			alarm_time[1][1] = TIME[4];
			alarm_time[1][2] = TIME[5];
			
		}
		ds1302_read_all_time();//读一次时间到time全局变量 防止意外
		run_state = show_state;
		timer0_count = 0;
		TR0 = 0;
		EX0 = 1;
		my_8bit_flag |= 0x0f;//退出时清除选项按键缓存
		key_pressed = 0;
		my_8bit_flag&=0xf7;
	}
}

void bcd_add_1(unsigned char *bcd_dat,unsigned char position)
{
	bcd_dat[position]++;
	if((bcd_dat[position]&0x0f)>9)					 //BCD进位方式
	{
		bcd_dat[position]=bcd_dat[position]+6;
	}
	switch(position)
	{
		case 0:		//秒
			if(bcd_dat[position] >= 0x60)		//59 秒
				bcd_dat[position] &= 0x80;		//秒钟清零
			break;
		case 1:		//分
			if(bcd_dat[position] >= 0x60)		//59 分
				bcd_dat[position] &= 0x80;		//分钟清零
			break;
		case 2:		//时
			if(bcd_dat[position] >= 0x24)		//23 时
				bcd_dat[position] &= 0xc0;		//时钟清零
			break;
		case 3:		//日
			if(bcd_dat[position] >= 0x32)		//31 日
				bcd_dat[position] &= 0xc0;		//日清零
			break;
		case 4:		//月
			if(bcd_dat[position] >= 0x13)		//12 月
				bcd_dat[position] &= 0xe0;		//月清零
			break;
		case 5:		//周
			if(bcd_dat[position] >= 0x07)		// 7 周
				bcd_dat[position] &= 0xf0;		//周清零
			break;
		case 6:		//年
			if(bcd_dat[position] > 0x99)		//99 年
				bcd_dat[position] &= 0x00;		//年清零
			break;
	}	
}

void delay1ms()		//@12.000MHz
{
	unsigned char i, j;

	i = 12;
	j = 169;
	do
	{
		while (--j);
	} while (--i);
}

void alarm_show(void)
{
	temp_buf[0]=(alarm_time[0][2]&0x30)/16 + '0';
	temp_buf[1]=(alarm_time[0][2]&0x0f) + '0';    //时
	temp_buf[2]=':';
	temp_buf[3]=(alarm_time[0][1]&0x70)/16 + '0';
	temp_buf[4]=(alarm_time[0][1]&0x0f) + '0';//分
	temp_buf[5]='\0';
	oled_ascii_8x16_str(58,2,temp_buf);
	temp_buf[0]=(alarm_time[1][2]&0x30)/16 + '0';
	temp_buf[1]=(alarm_time[1][2]&0x0f) + '0';    //时
	temp_buf[2]=':';
	temp_buf[3]=(alarm_time[1][1]&0x70)/16 + '0';
	temp_buf[4]=(alarm_time[1][1]&0x0f) + '0';//分
	temp_buf[5]='\0';
	oled_ascii_8x16_str(58,4,temp_buf);
	if(alarm_time[0][0] == 0x00)
		oled_ascii_8x16_str(104,2,"OFF");
	else
		oled_ascii_8x16_str(104,2," ON");
	if(alarm_time[1][0] == 0x00)
		oled_ascii_8x16_str(104,4,"OFF");
	else
		oled_ascii_8x16_str(104,4," ON");
}