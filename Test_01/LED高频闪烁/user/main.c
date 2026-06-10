
#include "stm32f10x.h"
#include "sys.h"
#include "delay.h"
#include "usart3.h"
#include "LED.h"
#include "SysTick.h"
#include "PWM.h"
int i=0;
int main()
{
	USART3_Init(9600);
	//LED_init();
	delay_init();
	PWM_Init1(999,719);
	//SysTick_Init();
	//SysTick->CTRL |=  SysTick_CTRL_ENABLE_Msk;	 //使能总算法时钟

	TIM_SetCompare1(TIM2,880);//PA0//0~999
	TIM_SetCompare2(TIM2,880);//PA1
				 
	TIM_SetCompare3(TIM2,880);//PA2

	while(1)
	{			
//		for(i=0;i<14;i++)//16
//		{
//			TIM_SetCompare3(TIM2,700);		
//			delay_ms(15);
//			TIM_SetCompare3(TIM2,999);
//			delay_ms(15);
//		}
//		TIM_SetCompare3(TIM2,880);
//		for(i=0;i<8;i++)
//		{
//			delay_ms(1000);
//		}
		/*
		if(flag_led==1)
		{
			TIM_SetCompare1(TIM2,0); 
		}
		if(flag_led==2)
		{
			TIM_SetCompare1(TIM2,999);
		}
		*/

		if(flag_led==1)
		{
			if(flag==1)
			{
				//flag_led=0;
				//flag=0;
				
				for(i=0;i<14;i++)//16
				{
					TIM_SetCompare1(TIM2,600);		
					delay_ms(15);
					TIM_SetCompare1(TIM2,999);
					delay_ms(15);
				}
				TIM_SetCompare1(TIM2,880);
				for(i=0;i<flag_num;i++)
				{
					delay_ms(1000);
				}
				
				//flag_num=0;
				
			}
		}
		
		if(flag_led==2)
		{
			if(flag==1)
			{
				//flag_led=0;
				//flag=0;
				for(i=0;i<14;i++)//16
				{
					TIM_SetCompare2(TIM2,600);		
					delay_ms(15);
					TIM_SetCompare2(TIM2,999);
					delay_ms(15);
				}
				TIM_SetCompare2(TIM2,880);
				for(i=0;i<flag_num;i++)
				{
					delay_ms(1000);
				}
				//flag_num=0;
			}
		}
		
		if(flag_led==3)
		{
			if(flag==1)
			{
				//flag_led=0;
				//flag=0;
				for(i=0;i<14;i++)//16
				{
					TIM_SetCompare3(TIM2,600);		
					delay_ms(15);
					TIM_SetCompare3(TIM2,999);
					delay_ms(15);
				}
				TIM_SetCompare3(TIM2,880);
				for(i=0;i<flag_num;i++)
				{
					delay_ms(1000);
				}
				//flag_num=0;
			}
		}
		
		
		
	}
	
}



//测试LED闪烁 PA0 ? 5 ? ? PA1 ? 5 ? ? PA2 ? 5 ? ? ?????????,?????
/*
#include "stm32f10x.h"
#include "sys.h"
#include "delay.h"
#include "PWM.h"

int main()
{
    int i;                          // ???????????(C89)
    
    delay_init();
    PWM_Init1(999, 719);            // 100Hz PWM, PA0/PA1/PA2

    while(1)
    {
        // PA0 ?? 5 ?
        for(i = 0; i < 5; i++)
        {
            TIM_SetCompare1(TIM2, 0);     // PA0 ?
            delay_ms(200);
            TIM_SetCompare1(TIM2, 999);   // PA0 ?
            delay_ms(200);
        }

        // PA1 ?? 5 ?
        for(i = 0; i < 5; i++)
        {
            TIM_SetCompare2(TIM2, 0);     // PA1 ?
            delay_ms(200);
            TIM_SetCompare2(TIM2, 999);   // PA1 ?
            delay_ms(200);
        }

        // PA2 ?? 5 ?
        for(i = 0; i < 5; i++)
        {
            TIM_SetCompare3(TIM2, 0);     // PA2 ?
            delay_ms(200);
            TIM_SetCompare3(TIM2, 999);   // PA2 ?
            delay_ms(200);
        }

        delay_ms(1000);  // ? 1 ?????
    }
}
*/


