#include "usart3.h"
#include "delay.h"
#include "stdio.h"
#include <stdarg.h>
#include <stdlib.h>

u8 flag_led = 0;
u8 flag_num = 0;
u8 flag = 0;


/**********************************
USART初始化 
**********************************/
void USART3_Init(u32 bound)
{
    GPIO_InitTypeDef GPIO_InitStrue;
	USART_InitTypeDef USART_InitStrue;
	NVIC_InitTypeDef NVIC_InitStrue;
	
	// 外设使能时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);
	USART_DeInit(USART3);  //复位串口2 -> 可以没有
	
	// 初始化 串口对应IO口  TX-PB10  RX-PB11
	GPIO_InitStrue.GPIO_Mode=GPIO_Mode_AF_PP;
	GPIO_InitStrue.GPIO_Pin=GPIO_Pin_10;
	GPIO_InitStrue.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStrue);
	
	GPIO_InitStrue.GPIO_Mode=GPIO_Mode_IN_FLOATING;
	GPIO_InitStrue.GPIO_Pin=GPIO_Pin_11;
    GPIO_Init(GPIOB,&GPIO_InitStrue);
	
	// 初始化 串口模式状态
	USART_InitStrue.USART_BaudRate=bound; // 波特率
	USART_InitStrue.USART_HardwareFlowControl=USART_HardwareFlowControl_None; // 硬件流控制
	USART_InitStrue.USART_Mode=USART_Mode_Tx|USART_Mode_Rx; // 发送 接收 模式都使用
	USART_InitStrue.USART_Parity=USART_Parity_No; // 没有奇偶校验
	USART_InitStrue.USART_StopBits=USART_StopBits_1; // 一位停止位
	USART_InitStrue.USART_WordLength=USART_WordLength_8b; // 每次发送数据宽度为8位
	USART_Init(USART3,&USART_InitStrue);
	
	USART_Cmd(USART3,ENABLE);//使能串口
	USART_ITConfig(USART3,USART_IT_RXNE,ENABLE);//开启接收中断
	
	// 初始化 中断优先级
	NVIC_InitStrue.NVIC_IRQChannel=USART3_IRQn;
	NVIC_InitStrue.NVIC_IRQChannelCmd=ENABLE;
	NVIC_InitStrue.NVIC_IRQChannelPreemptionPriority=1;
	NVIC_InitStrue.NVIC_IRQChannelSubPriority=1;
	NVIC_Init(&NVIC_InitStrue);

}

/**********************************
串口发送数据  
**********************************/
void Usart3_send(u8 com)
{	
	USART_ClearFlag(USART3,USART_FLAG_TC);
	USART_SendData(USART3,com);
	while(!USART_GetFlagStatus(USART3,USART_FLAG_TC));
}

void uart3_senddata(u8 ch)
{
		USART_SendData(USART3, ch);
		while( USART_GetFlagStatus(USART3, USART_FLAG_TXE)==0);
}
void printf3(char *fmt, ...) 
{ 
    char buffer[CMD_BUFFER_LEN+1]; 
    u8 i = 0; 
    va_list arg_ptr; 
    va_start(arg_ptr, fmt);   
    vsnprintf(buffer, CMD_BUFFER_LEN+1, fmt, arg_ptr); 
    while ((i < CMD_BUFFER_LEN) && buffer[i]) 
    { 
            USART_SendData(USART3, (u8) buffer[i++]); 
    while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);  
    } 
}
/////////////////////////////////////////////////


void USART3_IRQHandler(void) // 串口2中断服务函数
{
	u8 res;
	if(USART_GetITStatus(USART3,USART_IT_RXNE)) // 中断标志
    {
     res= USART_ReceiveData(USART3);  // 串口3 接收
     //USART_SendData(USART3,res);   // 串口3 发送
		
		/****************选择灯****************/
		if(res==0xff)
		{
			flag_led=0;
			printf3("close led");
		}
		if(res==0x0a)
		{
			flag_led=1;
			printf3("A");
		}
		if(res==0x0b)
		{
			flag_led=2;
			printf3("B");
		}
		if(res==0x0c)
		{
			flag_led=3;
			printf3("C");
		}
		
		/****************开始****************/
		if(res==0x99)
		{
			flag=1;
			printf3("99");
		}
		
		/****************数字读取********************/
		if(res==0x00)
		{
			flag_num=0;
			printf3("0");
		}
		if(res==0x01)
		{
			flag_num=1;
			printf3("1");
		}
		if(res==0x02)
		{
			flag_num=2;
			printf3("2");
		}
		if(res==0x03)
		{
			flag_num=3;
			printf3("3");
		}
		if(res==0x04)
		{
			flag_num=4;
			printf3("4");
		}
		if(res==0x05)
		{
			flag_num=5;
			printf3("5");
		}
		if(res==0x06)
		{
			flag_num=6;
			printf3("6");
		}
		if(res==0x07)
		{
			flag_num=7;
			printf3("7");
		}
		if(res==0x08)
		{
			flag_num=8;
			printf3("8");
		}
		if(res==0x09)
		{
			flag_num=9;
			printf3("9");
		}				
  }
}

