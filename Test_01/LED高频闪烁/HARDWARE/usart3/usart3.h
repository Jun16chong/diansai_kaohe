#ifndef __USART3_H
#define __USART3_H

#include "stm32f10x.h"
#include "stm32f10x_usart.h"

#define CMD_BUFFER_LEN 100
///////////////////////
extern u8 flag_led;
extern u8 flag_num;
extern u8 flag;
/////////////////////

void USART3_Init(u32 bound);
void Usart3_send(u8 com);
void uart3_senddata(u8 ch);
void printf3(char *fmt, ...);
#endif  

