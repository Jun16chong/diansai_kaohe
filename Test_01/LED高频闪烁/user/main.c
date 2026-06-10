/**
 * 可见光室内定位 — LED 控制电路
 * 功能: 4×4 矩阵键盘输入数字 → LED 闪烁编码发送
 * 硬件: STM32F103C8 + 3×白光 LED (PA0/PA1/PA2, TIM2 PWM)
 *
 * 键盘布局:
 *   1   2   3   A
 *   4   5   6   B
 *   7   8   9   C
 *   *   0   #   D
 *
 * 操作流程:
 *   按 A/B/C → 选择要闪烁的 LED
 *   按 0~9  → 输入要发送的数字
 *   按 #    → 确认发送 (LED 闪烁对应次数)
 *   按 *    → 取消/停止
 */

#include "stm32f10x.h"
#include "sys.h"
#include "delay.h"
#include "usart3.h"
#include "LED.h"
#include "SysTick.h"
#include "PWM.h"
#include "keypad.h"

int  i = 0;
u8   input_num  = 0;      /* 键盘输入的数字 (0~9) */
u8   input_ready = 0;     /* 输入完成标志: 收到数字后置 1, # 确认后清零 */

int main()
{
    u8 key;

    /* 外设初始化 */
    USART3_Init(9600);         /* UART 调试/备用控制 */
    delay_init();
    PWM_Init1(999, 719);       /* 100Hz PWM, PA0/PA1/PA2 */
    Keypad_Init();             /* 矩阵键盘 */

    /* LED 默认常亮 (88% 占空比) */
    TIM_SetCompare1(TIM2, 880);  /* PA0 — LED A */
    TIM_SetCompare2(TIM2, 880);  /* PA1 — LED B */
    TIM_SetCompare3(TIM2, 880);  /* PA2 — LED C */

    printf3("=== LED Control Ready ===\r\n");
    printf3("A/B/C=Select LED, 0-9=Number, #=Send, *=Stop\r\n");

    while(1)
    {
        /* ---- 键盘扫描 ---- */
        key = Keypad_GetKey();

        switch (key)
        {
            /* 选择 LED */
            case KEY_A:
                flag_led = 1;
                printf3("LED A selected\r\n");
                break;

            case KEY_B:
                flag_led = 2;
                printf3("LED B selected\r\n");
                break;

            case KEY_C:
                flag_led = 3;
                printf3("LED C selected\r\n");
                break;

            /* 输入数字 */
            case KEY_0:
                input_num = 0;
                input_ready = 1;
                printf3("Input: 0\r\n");
                break;
            case KEY_1:
                input_num = 1;
                input_ready = 1;
                printf3("Input: 1\r\n");
                break;
            case KEY_2:
                input_num = 2;
                input_ready = 1;
                printf3("Input: 2\r\n");
                break;
            case KEY_3:
                input_num = 3;
                input_ready = 1;
                printf3("Input: 3\r\n");
                break;
            case KEY_4:
                input_num = 4;
                input_ready = 1;
                printf3("Input: 4\r\n");
                break;
            case KEY_5:
                input_num = 5;
                input_ready = 1;
                printf3("Input: 5\r\n");
                break;
            case KEY_6:
                input_num = 6;
                input_ready = 1;
                printf3("Input: 6\r\n");
                break;
            case KEY_7:
                input_num = 7;
                input_ready = 1;
                printf3("Input: 7\r\n");
                break;
            case KEY_8:
                input_num = 8;
                input_ready = 1;
                printf3("Input: 8\r\n");
                break;
            case KEY_9:
                input_num = 9;
                input_ready = 1;
                printf3("Input: 9\r\n");
                break;

            /* 确认发送 */
            case KEY_HASH:
                if (input_ready && flag_led != 0)
                {
                    flag_num = input_num;
                    flag = 1;
                    printf3("Send: LED=%d, Num=%d\r\n", flag_led, flag_num);
                    input_ready = 0;
                }
                break;

            /* 停止 / 清除 */
            case KEY_STAR:
                flag_led = 0;
                flag = 0;
                flag_num = 0;
                input_num = 0;
                input_ready = 0;
                printf3("Stop / Clear\r\n");
                break;

            default:
                break;
        }

        /* ---- LED 闪烁执行 (数字编码: 闪烁 N 次 = 数字 N) ---- */
        if (flag == 1 && flag_led >= 1 && flag_led <= 3)
        {
            u8 blink_cnt = flag_num;        /* 闪烁次数 = 要发送的数字 */

            printf3("Blink LED%d x %d times...\r\n", flag_led, blink_cnt);

            /* 先短暂全暗 200ms → 告诉 K230 "准备接收" */
            TIM_SetCompare1(TIM2, 0);
            TIM_SetCompare2(TIM2, 0);
            TIM_SetCompare3(TIM2, 0);
            delay_ms(200);

            /* 恢复非目标 LED 的亮度 */
            if (flag_led != 1) TIM_SetCompare1(TIM2, 880);
            if (flag_led != 2) TIM_SetCompare2(TIM2, 880);
            if (flag_led != 3) TIM_SetCompare3(TIM2, 880);

            delay_ms(200);

            /* 目标 LED 精确闪烁 blink_cnt 次 */
            /* 每闪一次: 灭 100ms → 亮 100ms */
            for (i = 0; i < blink_cnt; i++)
            {
                /* 灭 */
                if (flag_led == 1) TIM_SetCompare1(TIM2, 0);
                if (flag_led == 2) TIM_SetCompare2(TIM2, 0);
                if (flag_led == 3) TIM_SetCompare3(TIM2, 0);
                delay_ms(100);

                /* 亮 */
                if (flag_led == 1) TIM_SetCompare1(TIM2, 880);
                if (flag_led == 2) TIM_SetCompare2(TIM2, 880);
                if (flag_led == 3) TIM_SetCompare3(TIM2, 880);
                delay_ms(100);
            }

            /* 全部恢复常亮 */
            TIM_SetCompare1(TIM2, 880);
            TIM_SetCompare2(TIM2, 880);
            TIM_SetCompare3(TIM2, 880);

            printf3("LED%d done (%d blinks)\r\n", flag_led, blink_cnt);

            flag = 0;
            flag_led = 0;
            flag_num = 0;
        }
    }
}
