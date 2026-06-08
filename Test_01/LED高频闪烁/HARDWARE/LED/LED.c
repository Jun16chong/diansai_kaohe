#include "LED.h"
#include "delay.h"
#include "SysTick.h"

u8 t=0;

void LED_init(void)
{	
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
		
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}


void LED1_50hz(void)
{
	for(t=0;t<100;t++)//循环5个周期，共100ms
	{
//		if(time%2==0)
//		{
//			PBout(7)=1;
//		}
//		if(time%2==1)
//		{
//			PBout(7)=0;
//		}
		PBout(7)=0;
		delay_ms(5);
		PBout(7)=1;
		delay_ms(5);

	}
	PBout(7)=1;
}

void LED2_50hz(void)
{
	for(t=0;t<5;t++)//循环5个周期，共100ms
	{
//		if(time%2==0)
//		{
//			PBout(8)=1;
//		}
//		if(time%2==1)
//		{
//			PBout(8)=0;
//		}
		PBout(8)=1;
		delay_ms(10);
		PBout(8)=0;
		delay_ms(10);
	}
	PBout(8)=1;
}

void LED3_50hz(void)
{
	for(t=0;t<5;t++)//循环5个周期，共100ms
	{
//		if(time%2==0)
//		{
//			PBout(9)=1;
//		}
//		if(time%2==1)
//		{
//			PBout(9)=0;
//		}
		PBout(9)=1;
		delay_ms(10);
		PBout(9)=0;
		delay_ms(10);
	}
	PBout(9)=1;
}

