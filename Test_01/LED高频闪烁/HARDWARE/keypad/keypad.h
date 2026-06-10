#ifndef __KEYPAD_H
#define __KEYPAD_H

#include "stm32f10x.h"
#include "sys.h"

/* 4×4 矩阵键盘引脚定义 */
/* 行 (ROW) — PA4~PA7, 推挽输出 */
#define KEYPAD_ROW_PORT     GPIOA
#define KEYPAD_ROW1_PIN     GPIO_Pin_4
#define KEYPAD_ROW2_PIN     GPIO_Pin_5
#define KEYPAD_ROW3_PIN     GPIO_Pin_6
#define KEYPAD_ROW4_PIN     GPIO_Pin_7
#define KEYPAD_ROW_PINS     (KEYPAD_ROW1_PIN | KEYPAD_ROW2_PIN | \
                             KEYPAD_ROW3_PIN | KEYPAD_ROW4_PIN)

/* 列 (COL) — PB12~PB15, 上拉输入 */
#define KEYPAD_COL_PORT     GPIOB
#define KEYPAD_COL1_PIN     GPIO_Pin_12
#define KEYPAD_COL2_PIN     GPIO_Pin_13
#define KEYPAD_COL3_PIN     GPIO_Pin_14
#define KEYPAD_COL4_PIN     GPIO_Pin_15
#define KEYPAD_COL_PINS     (KEYPAD_COL1_PIN | KEYPAD_COL2_PIN | \
                             KEYPAD_COL3_PIN | KEYPAD_COL4_PIN)

/* 按键返回值 */
#define KEY_NONE    0xFF    /* 无按键按下 */
#define KEY_0       0x00
#define KEY_1       0x01
#define KEY_2       0x02
#define KEY_3       0x03
#define KEY_4       0x04
#define KEY_5       0x05
#define KEY_6       0x06
#define KEY_7       0x07
#define KEY_8       0x08
#define KEY_9       0x09
#define KEY_A       0x0A    /* 选择 LED A */
#define KEY_B       0x0B    /* 选择 LED B */
#define KEY_C       0x0C    /* 选择 LED C */
#define KEY_STAR    0x0D    /* * 键 — 清除/停止 */
#define KEY_HASH    0x0E    /* # 键 — 确认/开始发送 */

void Keypad_Init(void);
u8   Keypad_Scan(void);
u8   Keypad_GetKey(void);   /* 带消抖的单次按键读取 */

#endif
