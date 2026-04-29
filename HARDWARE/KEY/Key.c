#include "stm32f10x.h"
#include "Delay.h"

/*
函数名: Key_Init
功能: 初始化按键GPIO(WK_UP/PA0下拉, PC9/PC8上拉)
使用方法: 在系统初始化时调用一次
作者: 胡荣杰
时间: 2025年12月17日
*/
void Key_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 使能GPIOA和GPIOC时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC, ENABLE);

    /* 配置WK_UP (PA0) - 下拉输入 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;  // 下拉输入(按下=高电平)
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* 配置SW3/SW4 (PC9/PC8) - 上拉输入 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入(按下=低电平)
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

/*
函数名: Key_GetNum
功能: 检测按键按下并返回键值
使用方法: 周期性调用,返回1=SW2, 2=SW3, 3=SW4, 0=无按键
作者: 胡荣杰
时间: 2025年12月17日
*/
uint8_t Key_GetNum(void)
{
    uint8_t KeyNum = 0;

    /* 检测SW2 (WK_UP/PA0) - 按下时为高电平 */
    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 1)
    {
        delay_ms(20);  // 消抖
        if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 1)
        {
            while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 1);  // 等待松开
            KeyNum = 1;
        }
    }

    /* 检测SW3 (PC9) - 按下时为低电平 */
    if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_9) == 0)
    {
        delay_ms(20);  // 消抖
        if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_9) == 0)
        {
            while (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_9) == 0);  // 等待松开
            KeyNum = 2;
        }
    }

    /* 检测SW4 (PC8) - 按下时为低电平 */
    if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8) == 0)
    {
        delay_ms(20);  // 消抖
        if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8) == 0)
        {
            while (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8) == 0);  // 等待松开
            KeyNum = 3;
        }
    }

    return KeyNum;
}
