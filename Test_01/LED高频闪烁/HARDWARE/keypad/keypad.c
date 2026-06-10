/**
 * 4×4 矩阵键盘驱动
 * 行: PA4~PA7 (推挽输出), 列: PB12~PB15 (上拉输入)
 *
 * 按键布局:
 *   1   2   3   A
 *   4   5   6   B
 *   7   8   9   C
 *   *   0   #   D
 *
 * 扫描原理: 轮流拉低一行, 读取列电平 — 被拉低的那列即对应按键按下
 */

#include "keypad.h"
#include "delay.h"

/* 按键映射表 [行][列] */
static const u8 key_map[4][4] = {
    { KEY_1,    KEY_2,    KEY_3,    KEY_A    },   /* ROW1 (PA4) */
    { KEY_4,    KEY_5,    KEY_6,    KEY_B    },   /* ROW2 (PA5) */
    { KEY_7,    KEY_8,    KEY_9,    KEY_C    },   /* ROW3 (PA6) */
    { KEY_STAR, KEY_0,    KEY_HASH, KEY_NONE },   /* ROW4 (PA7), D 键未用 */
};

/* 对应每一行的 GPIO 引脚 */
static const u16 row_pins[4] = {
    KEYPAD_ROW1_PIN,
    KEYPAD_ROW2_PIN,
    KEYPAD_ROW3_PIN,
    KEYPAD_ROW4_PIN,
};

/* 对应每一列的 GPIO 引脚 */
static const u16 col_pins[4] = {
    KEYPAD_COL1_PIN,
    KEYPAD_COL2_PIN,
    KEYPAD_COL3_PIN,
    KEYPAD_COL4_PIN,
};

/**
 * 初始化矩阵键盘 GPIO
 * 行 → 推挽输出, 默认高电平
 * 列 → 上拉输入
 */
void Keypad_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 使能 GPIOA, GPIOB 时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* 行引脚 PA4~PA7 → 推挽输出, 默认高 */
    GPIO_InitStructure.GPIO_Pin  = KEYPAD_ROW_PINS;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(KEYPAD_ROW_PORT, &GPIO_InitStructure);

    /* 所有行初始拉高 */
    GPIO_SetBits(KEYPAD_ROW_PORT, KEYPAD_ROW_PINS);

    /* 列引脚 PB12~PB15 → 上拉输入 */
    GPIO_InitStructure.GPIO_Pin  = KEYPAD_COL_PINS;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(KEYPAD_COL_PORT, &GPIO_InitStructure);
}

/**
 * 原始扫描 — 返回当前按下的键值
 * 无按键时返回 KEY_NONE (0xFF)
 */
u8 Keypad_Scan(void)
{
    u8 row, col;

    for (row = 0; row < 4; row++)
    {
        /* 所有行先拉高 */
        GPIO_SetBits(KEYPAD_ROW_PORT, KEYPAD_ROW_PINS);

        /* 拉低当前行 */
        GPIO_ResetBits(KEYPAD_ROW_PORT, row_pins[row]);

        /* 短暂延时让电平稳定 */
        delay_us(10);

        /* 逐列检测 */
        for (col = 0; col < 4; col++)
        {
            if (GPIO_ReadInputDataBit(KEYPAD_COL_PORT, col_pins[col]) == 0)
            {
                /* 恢复行电平后返回 */
                GPIO_SetBits(KEYPAD_ROW_PORT, row_pins[row]);
                return key_map[row][col];
            }
        }

        /* 恢复当前行 */
        GPIO_SetBits(KEYPAD_ROW_PORT, row_pins[row]);
    }

    return KEY_NONE;
}

/**
 * 带消抖的按键读取
 * 按下消抖 + 等待释放 + 释放消抖
 * 返回键值, 无按键时返回 KEY_NONE
 */
u8 Keypad_GetKey(void)
{
    u8 key;

    key = Keypad_Scan();
    if (key == KEY_NONE)
        return KEY_NONE;

    /* 按下消抖 20ms */
    delay_ms(20);
    if (Keypad_Scan() != key)
        return KEY_NONE;    /* 抖动, 丢弃 */

    /* 等待释放 (简单消抖: 200ms 超时) */
    {
        u16 timeout = 0;
        while (Keypad_Scan() != KEY_NONE && timeout < 400)
        {
            delay_ms(5);
            timeout++;
        }
    }

    /* 释放消抖 10ms */
    delay_ms(10);

    return key;
}
