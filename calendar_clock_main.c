#include <reg52.h>
#include "ds1302_driver.h"
#include "oled_iic_driver.h"

/*³ÌĞòÔËĞĞ×´Ì¬±êÖ¾*/
#define show_state 0		//ÏÔÊ¾Ä£Ê½
#define menu_state  1        //²Ëµ¥Ä£Ê½
#define set_alarm  2        //ÄÖÖÓÉèÖÃ
#define set_time   3   		//Ê±¼äÉèÖÃ
#define alarm_noisy 4

//°´¼ü°´ÏÂµÄµçÆ½×´Ì¬
#define key_down 0			
#define key_up   1

sbit menu_key = P2^7;//½«°´¼üÌøµ½P3^2 Íâ²¿ÖĞ¶Ï0ÊäÈë
sbit shift_key = P2^6;
sbit add_key = P2^5;
sbit save_key = P2^4;
sbit alarm = P3^4;

unsigned char temp_buf[10];		//»º³åÇø
unsigned char alarm_time[2][3] ={{0x00, 0x30,0x08},{0x00, 0x20,0x08}};//ÄÖÖÓÊ±¼ä2×é¿ª¹Ø·ÖÊ±bcdÂë´æ·Å
unsigned char *time_ascii;				//ÓÃÓÚ´æ·Å×ª»»³ÉasciiºóµÄÊ±¼äÊı¾İ
unsigned char run_state = show_state;	//³õÊ¼»¯³ÌĞòÔËĞĞ×´Ì¬ÎªÏÔÊ¾Ä£Ê½
unsigned char timer0_count = 1;			//¶¨Ê±Æ÷Òç³ö´ÎÊı
unsigned char key_pressed = 0;			//ÓĞÎŞ°´¼ü°´ÏÂ
unsigned char tc = 0;					//ÓÃÓÚÏû¶¶
unsigned char my_8bit_flag = 0x00;		//8Î»±êÖ¾Î»
										//ÏÔÊ¾±êÖ¾ ²Ëµ¥±êÖ¾ ÉèÄÖÖÓ±êÖ¾ ÉèÊ±¼ä±êÖ¾ 
										//ÉÁË¸±êÖ¾ alarm Ñ¡ÖĞtime set  Ñ¡ÖĞalarm set
int position = 0;//ÉèÖÃÊ±¼äÊ±ºòµÄ¹â±êÎ»ÖÃ
unsigned char blink_count = 0;

//***********************************************************************************
void alarm_show(void);

void delay1ms(void);
	
unsigned char* ds1302_time_dat_to_ascii(unsigned char time[]);/* *½«ds1302¶Á³öµÄÊı¾İ×ª»»ÎªasciiÂë*/	

void show_time(unsigned char ascii_time[],unsigned char page_drift);/* *ÓÃÓÚÏÔÊ¾Ê±¼ä*/

void bcd_add_1(unsigned char *bcd_dat,unsigned char position);//ÉèÖÃÊ±¼ä»òÄÖÖÓÊ±µ÷ÓÃ£¬¶ÔÓ¦Î»ÖÃµÄbcdÂë¼ÓÒ»

void Timer0Init(void);//50ºÁÃë¶¨Ê±Æ÷0³õÊ¼»¯@12.000MHz		


void main(void)
{
	
	P3 |= 0x10;//ÆÁ±Î·³ÔêÈËµÄ·äÃùÆ÷
	//ds1302_init();
	oled_init();
	Timer0Init();
	IT0=0; //Íâ²¿ÖĞ¶Ï0µÍµçÆ½ÓĞĞ§
	EX0=1; //Íâ²¿ÖĞ¶Ï¿ªÆô
	ET0=1; //¿ªÆô¶¨Ê±Æ÷0ÖĞ¶Ï
	TR0 = 0;
	EA=1;  //×ÜÖĞ¶Ï¿ªÆô
	while(1)
	{
		switch(run_state)
		{
			case show_state:
				if((my_8bit_flag&0x80)==0x00)//¸Õ½øÀ´Ê± ÇåÒ»´ÎÆÁ
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
				if((my_8bit_flag&0x40)==0x00)//¸Õ½øÀ´Ê± ÇåÒ»´ÎÆÁÖ»ÏÔÊ¾Ò»´Î²Ëµ¥£¬±ÜÃâÑ­»·ÏÔÊ¾¶Ô³ÌĞò´øÀ´½Ï´óÑÓÊ±
				{	
					oled_all_fill(0x00);
					oled_ascii_8x16_str(12,0,"<Option Menu>\0");
					oled_ascii_8x16_str(2,2,"1.Time Set\0");
					oled_ascii_8x16_str(2,4,"2.Alarm Set\0");
					oled_ascii_8x16_str(0,6,"@Author:CTBU^Shy\0");
					my_8bit_flag |= 0x40;
				}
				/* *°´¼ü¼ì²â */
				if((key_pressed==0)&&((shift_key==key_down)||(add_key==key_down)||(save_key==key_down)))
				{
					
					tc = 20;//ÓÃÓÚÏû¶¶
					key_pressed = 1;
				}
				delay1ms();
				if(key_pressed)
				{
					tc--;
					if(!tc)//Ïû¶¶½áÊø
					{
						if(add_key == key_down)//add_key°´ÏÂ
						{
							my_8bit_flag |= 0x01;//alarm set
							my_8bit_flag &= 0xfd;//off time set
							timer0_count = 0;
							key_pressed = 0;
							oled_ascii_8x16_str(80,2,"    \0");
							oled_ascii_8x16_str(88,4,"<-<-\0");
						}
						else if(shift_key == key_down)//shift_key°´ÏÂ
						{
							my_8bit_flag |= 0x02;//time set
							my_8bit_flag &= 0xfe;//off alram set
							timer0_count = 0;
							key_pressed = 0;
							oled_ascii_8x16_str(80,2,"<-<-\0");
							oled_ascii_8x16_str(88,4,"    \0");
						}
						else if(save_key == key_down)//save_key°´ÏÂ
						{
							if((my_8bit_flag&0x01) == 0x01)//alarm set
							{
								timer0_count = 0;
								TR0 = 1;
								EX0 = 0;
								my_8bit_flag |= 0x03;//ÍË³öÊ±Çå³ıÑ¡Ïî°´¼ü»º´æ
								run_state = set_alarm;
								key_pressed = 0;
								break;//ÍË³ö²Ëµ¥
							}
							else if((my_8bit_flag&0x02) == 0x02)//time set
							{
								timer0_count = 0;
								TR0 = 1;
								EX0 = 0;
								my_8bit_flag |= 0x03;//ÍË³öÊ±Çå³ıÑ¡Ïî°´¼ü»º´æ
								run_state = set_time;
								key_pressed = 0;	
								break;//ÍË³ö²Ëµ¥
								
							}
							else
								key_pressed = 0;
						}
							
						else
							key_pressed = 0;//ÅĞ¶¨Îª¶¶¶¯
							
					}
				}
				break;
			case set_alarm:
				if((my_8bit_flag&0x20)==0x00)//Ö»ÏÔÊ¾Ò»´Î²Ëµ¥£¬±ÜÃâÑ­»·ÏÔÊ¾¶Ô³ÌĞò´øÀ´½Ï´óÑÓÊ±
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
				/*ÉÁË¸ÉèÖÃÄÖÖÓµÄÊµÏÖ*///12.16
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
						temp_buf[1]=(alarm_time[1][2]&0x0f) + '0';    //Ê±
						temp_buf[2]='\0';
						oled_ascii_8x16_str(58,4,temp_buf);//µãÁÁÇ°Ò»Î»
						
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
							temp_buf[4]=(alarm_time[0][1]&0x0f) + '0';//·Ö
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
						temp_buf[4]=(alarm_time[0][1]&0x0f) + '0';//·Ö
						temp_buf[5]='\0';
						oled_ascii_8x16_str(58+24,2,&temp_buf[3]);
						if(my_8bit_flag&0x08)
						{
							temp_buf[0]=(alarm_time[0][2]&0x30)/16 + '0';
							temp_buf[1]=(alarm_time[0][2]&0x0f) + '0';    //Ê±
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
						temp_buf[1]=(alarm_time[0][2]&0x0f) + '0';    //Ê±
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
							temp_buf[4]=(alarm_time[1][1]&0x0f) + '0';//·Ö
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
						temp_buf[4]=(alarm_time[1][1]&0x0f) + '0';//·Ö
						temp_buf[5]='\0';
						oled_ascii_8x16_str(58+24,4,&temp_buf[3]);
						if(my_8bit_flag&0x08)
						{
							temp_buf[0]=(alarm_time[1][2]&0x30)/16 + '0';
							temp_buf[1]=(alarm_time[1][2]&0x0f) + '0';    //Ê±
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
				/* *°´¼ü¼ì²â */
				if((key_pressed==0)&&((shift_key==key_down)||(add_key==key_down)||(save_key==key_down)))
				{
					tc=2;
					key_pressed = 1;
				}
				delay1ms();
				if(key_pressed&&!(tc--))
				{
					if(add_key == key_down)//add_key°´ÏÂ
					{
						timer0_count = 0;
						key_pressed = 0;//¿ª·Å°´¼ü¶ÁÈë
						if(position < 3)
						{
							if(position == 0)
								alarm_time[0][0] = ~alarm_time[0][0];//¿ª¹Ø×´Ì¬¸Ä±ä
							else
								bcd_add_1(alarm_time[0],position);//Ê±¼äÉèÖÃ
						}
						else
						{
							if(position == 3)
								alarm_time[1][0] = ~alarm_time[1][0];//¿ª¹Ø×´Ì¬¸Ä±ä
							else
								bcd_add_1(alarm_time[1],position-3);//Ê±¼äÉèÖÃ
						}
						alarm_show();//Ë¢ĞÂÏÔÊ¾ÄÖÖÓ	
					}
					else if(shift_key == key_down)//shift_key°´ÏÂ
					{
						position++;
						if(position>5)//6¸öÉèÖÃÎ»
							position = 0;
						
						timer0_count = 0;
						
						key_pressed = 0;//¿ª·Å°´¼ü¶ÁÈ¡
					}
					else if(save_key == key_down)//save_key°´ÏÂ
					{
						key_pressed = 0;//¿ª·Å°´¼ü¶ÁÈ¡
						my_8bit_flag&=0xf7;
						run_state = show_state;//·µ»ØÏÔÊ¾Ê±¼äÄ£Ê½
					}
							
					else
						key_pressed = 0;//ÅĞ¶¨Îª¶¶¶¯
				}
				break;
			case set_time:
				/*ÉÁË¸ÉèÖÃÊ±¼äµÄÊµÏÖ*///12.16
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
						{//ÏÔÊ¾Ç°Ò»Î»ÉÁË¸µÄÖµ
						temp[0] = time_ascii[18];
						temp[1] = time_ascii[19];	
						temp[2] = '\0';
						oled_ascii_8x16_str(31+24+24,5,temp);//ÏÔÊ¾miao
						}
						
						if(my_8bit_flag&0x08)
						{

							temp[0] = time_ascii[2];
							temp[1] = time_ascii[3];	
							temp[2] = '\0';
							oled_ascii_8x16_str(16,2,temp);//ÏÔÊ¾Äê
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
							oled_ascii_8x16_str(16,2,temp);//ÏÔÊ¾Äê
						}
						if(my_8bit_flag&0x08)
						{
							oled_chinese_characters_16x16(10*8+16*2,2,WEEK[time_ascii[11]-'0'+1]);//¼¸
						}
						else
						{
							oled_ascii_8x16_str(10*8+16*2,2,"  ");
						}
						break;
					case 4:
						{
							oled_chinese_characters_16x16(10*8+16*2,2,WEEK[time_ascii[11]-'0'+1]);//¼¸
						}
						if(my_8bit_flag&0x08)
						{
							temp[0] = time_ascii[5];
							temp[1] = time_ascii[6];	
							temp[2] = '\0';
							oled_ascii_8x16_str(16+24,2,temp);//ÏÔÊ¾ÔÂ
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
							oled_ascii_8x16_str(16+24,2,temp);//ÏÔÊ¾ÔÂ
						}
						if(my_8bit_flag&0x08)
						{
							temp[0] = time_ascii[8];
							temp[1] = time_ascii[9];	
							temp[2] = '\0';
							oled_ascii_8x16_str(16+24+24,2,temp);//ÏÔÊ¾ÈÕ
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
							oled_ascii_8x16_str(16+24+24,2,temp);//ÏÔÊ¾ÈÕ
						}
						if(my_8bit_flag&0x08)
						{
							temp[0] = time_ascii[12];
							temp[1] = time_ascii[13];	
							temp[2] = '\0';
							oled_ascii_8x16_str(31,5,temp);//ÏÔÊ¾Ê±
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
							oled_ascii_8x16_str(31,5,temp);//ÏÔÊ¾Ê±
						}
						if(my_8bit_flag&0x08)
						{
							temp[0] = time_ascii[15];
							temp[1] = time_ascii[16];	
							temp[2] = '\0';
							oled_ascii_8x16_str(31+24,5,temp);//ÏÔÊ¾·Ö
						}
						else
						{
							oled_ascii_8x16_str(31+24,5,"  ");//ÏÔÊ¾·Ö
						}			
						break;
					case 0:
						{
							temp[0] = time_ascii[15];
							temp[1] = time_ascii[16];	
							temp[2] = '\0';
							oled_ascii_8x16_str(31+24,5,temp);//ÏÔÊ¾·Ö
						}
						if(my_8bit_flag&0x08)
						{
							temp[0] = time_ascii[18];
							temp[1] = time_ascii[19];	
							temp[2] = '\0';
							oled_ascii_8x16_str(31+24+24,5,temp);//ÏÔÊ¾miao
						}
						else
						{
							oled_ascii_8x16_str(31+24+24,5,"  ");//ÏÔÊ¾miao
						}			
						break;	
					default:
						break;
				}
				
				if((my_8bit_flag&0x10)==0x00)//Ö»ÏÔÊ¾Ò»´Î²Ëµ¥£¬±ÜÃâÑ­»·ÏÔÊ¾¶Ô³ÌĞò´øÀ´½Ï´óÑÓÊ±
				{
					oled_all_fill(0x00);
					oled_ascii_8x16_str(12,0,"< Time  Set >\0");
					my_8bit_flag |= 0x10;
					ds1302_read_all_time();//¶ÁÒ»´ÎÊ±¼äµ½timeÈ«¾Ö±äÁ¿
					time_ascii = ds1302_time_dat_to_ascii(TIME);
					show_time(time_ascii,2);//ÏÔÊ¾Ò»´Îµ±Ç°µÄÊ±¼ä
					position = 6;//³õÊ¼»¯ÉèÖÃÄêÎ»ÖÃ
				}
				/* *°´¼ü¼ì²â */
				if((key_pressed==0)&&((shift_key==key_down)||(add_key==key_down)||(save_key==key_down)))
				{
					tc=2;
					key_pressed = 1;
				}
				delay1ms();
				if(key_pressed&&!(tc--))
				{
					if(add_key == key_down)//add_key°´ÏÂ
					{
						timer0_count = 0;
						bcd_add_1(TIME,position);
						key_pressed = 0;//¿ª·Å°´¼ü¶ÁÈë
						time_ascii = ds1302_time_dat_to_ascii(TIME);//Ë¢ĞÂÏÔÊ¾
						//show_time(time_ascii,2);
					}
					else if(shift_key == key_down)//shift_key°´ÏÂ
					{
						
						position--;
						if(position==-1)//7¸öÉèÖÃÎ»
							position = 6;
						timer0_count = 0;
						
						key_pressed = 0;//¿ª·Å°´¼ü¶ÁÈ¡
					}
					else if(save_key == key_down)//save_key°´ÏÂ
					{
						ds1302_init();//ÖØĞÂĞ´ÈëĞÂÉèÖÃµÄÊ±¼äµ½1302
						key_pressed = 0;//¿ª·Å°´¼ü¶ÁÈ¡
						position=0;
						my_8bit_flag&=0xf7;
						//my_8bit_flag&=0x7f;
						run_state = show_state;//·µ»ØÏÔÊ¾Ê±¼äÄ£Ê½
					}	
					else
						key_pressed = 0;//ÅĞ¶¨Îª¶¶¶¯
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
	//´æ·Å×ª»»ºóµÄasciiÂëÄê-ÔÂ-ÈÕ-ÖÜÊ±£º·Ö£ºÃë
	unsigned char* TIME = time;
	time_ascii[19] = (TIME[0]&0x0f) + '0';		//Ãë
	time_ascii[18] = (TIME[0]&0x70)/16 + '0';
	
	time_ascii[16] = (TIME[1]&0x0f) + '0';		//·Ö
	time_ascii[15] = (TIME[1]&0x70)/16 + '0';
		
	time_ascii[13] = (TIME[2]&0x0f) + '0';		//Ê±
	time_ascii[12] = (TIME[2]&0x30)/16 + '0';
		
	time_ascii[11] = (TIME[5]&0x07) + '1';		//ÖÜ
		
	time_ascii[9] = (TIME[3]&0x0f) + '0';		//ÈÕ
	time_ascii[8] = (TIME[3]&0x30)/16 + '0';
		
	time_ascii[6] = (TIME[4]&0x0f) + '0';		//ÔÂ
	time_ascii[5] = (TIME[4]&0x10)/16 + '0';
		
	time_ascii[3] = (TIME[6]&0x0f) + '0';		//Äê
	time_ascii[2] = (TIME[6]&0xf0)/16 + '0';
	
	time_ascii[1] =  '0';
	time_ascii[0] =  '2';
	return time_ascii;
}
void show_time(unsigned char ascii_time[],unsigned char page_drift)
{


	unsigned char* temp = ascii_time;
	temp[10] = '\0';//È¡ÄêÔÂÈÕ
	oled_ascii_8x16_str(0,page_drift,temp);//ÏÔÊ¾ÄêÔÂÈÕ
	oled_chinese_characters_16x16(10*8,page_drift,WEEK[0]);//ĞÇ
	oled_chinese_characters_16x16(10*8+16,page_drift,WEEK[1]);//ÆÚ
	oled_chinese_characters_16x16(10*8+16*2,page_drift,WEEK[ascii_time[11]-'0'+1]);//¼¸
	temp = ascii_time;
	temp = temp+12;//È¡Ê±Ê±·ÖÃë
	oled_ascii_8x16_str(31,3+page_drift,temp);//ÏÔÊ¾Ê±·ÖÃë
}

void Timer0Init(void)		//50ºÁÃë@12.000MHz
{
	TMOD = 0x01;		//ÉèÖÃ¶¨Ê±Æ÷Îª16Î»
	TL0 = 0xB0;		//ÉèÖÃ¶¨Ê±Æ÷³õÖµ
	TH0 = 0x3C;		//ÉèÖÃ¶¨Ê±Æ÷³õÖµ
	TF0 = 0;		//Çå³ıÖĞ¶Ï±êÖ¾Î»
	TR0 = 0;		//¹Ø±Õ¶¨Ê±Æ÷
}
void exint0(void) interrupt 0		//Íâ²¿ÖĞ¶Ï0µÄÖĞ¶Ï·şÎñº¯Êı
{
	TR0 = 1;//´ò¿ª¶¨Ê±Æ÷
	EX0 = 0;
}

void timer0(void) interrupt 1		//¶¨Ê±Æ÷0µÄÖĞ¶Ï·şÎñº¯Êı
{
	TL0 = 0xB0;		//ÉèÖÃ¶¨Ê±Æ÷³õÖµ
	TH0 = 0x3C;		//ÉèÖÃ¶¨Ê±Æ÷³õÖµ
	if(menu_key == key_down)//ÉèÖÃ°´¼üµÄÏû¶¶
	{
		
		if(run_state == show_state)
		{my_8bit_flag &= 0x0f;//¸ß4Î»±êÖ¾ÇåÁã
			run_state = menu_state;}
	}
	else
		EX0 = 1;
	if(run_state != show_state)
		timer0_count++;
	if(timer0_count >200)//Èç¹û10ÃëÃ»ÓĞÉèÖÃ²Ù×÷£¬ÍË³ö²Ëµ¥
	{
		
		if(run_state == set_alarm)//´ÓÉèÖÃÄÖÖÓÍË»ØÀ´
		{
			alarm_time[0][0] = TIME[0];//²»±£´æÉèÖÃ
			alarm_time[0][1] = TIME[1];
			alarm_time[0][2] = TIME[2];
			alarm_time[1][0] = TIME[3];
			alarm_time[1][1] = TIME[4];
			alarm_time[1][2] = TIME[5];
			
		}
		ds1302_read_all_time();//¶ÁÒ»´ÎÊ±¼äµ½timeÈ«¾Ö±äÁ¿ ·ÀÖ¹ÒâÍâ
		run_state = show_state;
		timer0_count = 0;
		TR0 = 0;
		EX0 = 1;
		my_8bit_flag |= 0x0f;//ÍË³öÊ±Çå³ıÑ¡Ïî°´¼ü»º´æ
		my_8bit_flag &= 0x0f;
		key_pressed = 0;
		my_8bit_flag&=0xf7;
	}
}

void bcd_add_1(unsigned char *bcd_dat,unsigned char position)
{
	bcd_dat[position]++;
	if((bcd_dat[position]&0x0f)>9)					 //BCD½øÎ»·½Ê½
	{
		bcd_dat[position]=bcd_dat[position]+6;
	}
	switch(position)
	{
		static unsigned char temp;
		case 0:		//Ãë
			if(bcd_dat[position] >= 0x60)		//59 Ãë
				bcd_dat[position] &= 0x80;		//ÃëÖÓÇåÁã
			break;
		case 1:		//·Ö
			if(bcd_dat[position] >= 0x60)		//59 ·Ö
				bcd_dat[position] &= 0x80;		//·ÖÖÓÇåÁã
			break;
		case 2:		//Ê±
			if(bcd_dat[position] >= 0x24)		//23 Ê±
				bcd_dat[position] &= 0xc0;		//Ê±ÖÓÇåÁã
			break;
		case 3:		//ÈÕ
			temp = bcd_dat[position+1]&0x1f;
			if(temp == 0x04||temp==0x06||temp==0x09||temp==0x11)
			{			//30 ã
				if(bcd_dat[position] >= 0x31)
					bcd_dat[position] &= 0xc1;
			}
			else if(temp == 0x02)
			{
				if((bcd_dat[position+3]&0xf0/16*10+bcd_dat[position+3]&0x0f)%4==0)
				{	
					if(bcd_dat[position] >= 0x30)
					{
						bcd_dat[position] &= 0xc1;
						bcd_dat[position]++;
					}
				}
				else
					if(bcd_dat[position] >= 0x29)
						bcd_dat[position] &= 0xc1;
				
			}
			else
			{
				if(bcd_dat[position] >= 0x32)
				{
					bcd_dat[position] &= 0xc1;
					bcd_dat[position]++;
				}
			}
			break;
		case 4:		//ÔÂ
			if(bcd_dat[position] >= 0x13)		//12 ÔÂ
				bcd_dat[position] &= 0xe1;		//ÔÂÇåÁã
			break;
		case 5:		//ÖÜ
			if(bcd_dat[position] >= 0x07)		// 7 ÖÜ
				bcd_dat[position] &= 0xf0;		//ÖÜÇåÁã
			break;
		case 6:		//Äê
			if(bcd_dat[position] > 0x99)		//99 Äê
				bcd_dat[position] &= 0x00;		//ÄêÇåÁã
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
	temp_buf[1]=(alarm_time[0][2]&0x0f) + '0';    //Ê±
	temp_buf[2]=':';
	temp_buf[3]=(alarm_time[0][1]&0x70)/16 + '0';
	temp_buf[4]=(alarm_time[0][1]&0x0f) + '0';//·Ö
	temp_buf[5]='\0';
	oled_ascii_8x16_str(58,2,temp_buf);
	temp_buf[0]=(alarm_time[1][2]&0x30)/16 + '0';
	temp_buf[1]=(alarm_time[1][2]&0x0f) + '0';    //Ê±
	temp_buf[2]=':';
	temp_buf[3]=(alarm_time[1][1]&0x70)/16 + '0';
	temp_buf[4]=(alarm_time[1][1]&0x0f) + '0';//·Ö
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