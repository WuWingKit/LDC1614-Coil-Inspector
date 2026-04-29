#ifndef __KEY_H__
#define __KEY_H__

#include "stm32f10x.h"

/* 按键定义 */
#define KEY_NONE    0    // 无按键按下
#define KEY_WKUP    1    // WK_UP按键 (SW2)
#define KEY_SW3     2    // SW3按键 (PC9)
#define KEY_SW4     3    // SW4按键 (PC8)

/* 函数声明 */
void Key_Init(void);
uint8_t Key_GetNum(void);

#endif
