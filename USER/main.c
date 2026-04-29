//////////////////////////////////////////////////////////////////////////////////
//  文 件 名   : main.c
//  版 本 号   : v14.5
//  作    者   : 胡荣杰
//  编写日期   : 2025-12-17
//  最近修改   : 2025-12-17 调整LCD屏幕刷新率,减少待机时的无效刷新
//  功能描述   : LDC1614电感式金属距离检测系统 - 合格件两点校准版
//
//  系统功能:
//  1. 两点校准法: 空载(100%)和合格件(50%)标定
//  2. 合格判定: 45%-55%显示PASS(绿色)
//  3. 实时测量: 显示距离百分比、原始值、进度条、状态
//  4. 滤波处理: 4点滑动平均,提高稳定性
//  5. 按键控制: 三键操作(测量/空载校准/合格件校准)
//
//  接口说明:
//  硬件平台: STM32F103系列
//  
//  LDC1614传感器:
//    VCC  → 3.3V电源
//    GND  → 电源地
//    SCL  → PB10 (I2C时钟)
//    SDA  → PB11 (I2C数据)
//    SD   → PC13 (关机控制,低电平工作)
//    ADDR → PA11 (地址选择,接GND=0x2A)
//    INTB → 悬空  (中断输出,未使用)
//
//  按键接口:
//    SW2(WK_UP) → PA0  (测量模式/确认校准)
//    SW3        → PC9  (合格件校准)
//    SW4        → PC8  (空载校准)
//
//  LED指示灯:
//    LED1 → PA8 (心跳指示)
//
//  串口调试:
//    TX → PA9  (115200,8,N,1)
//    RX → PA10
//
//  使用流程:
//  1. 上电启动,显示设备标题3秒
//  2. 初始化传感器,显示Ready
//  3. 按K3(SW4)进行空载校准(移除所有金属)
//  4. 按K4(SW3)进行合格件校准(放置标准工件)
//  5. 按K2(SW2)确认并开始测量
//  6. 实时显示距离: 45%-55%为PASS, >55%为FAR, <45%为NEAR
//
//  距离算法:
//  - 空载值(empty) = 100% (远端)
//  - 合格件(valid) = 50%  (中心标准)
//  - 推算近端     = 0%   (根据两点线性外推)
//  - distance = 100 - (current - empty) * 100 / ((valid - empty) * 2)
//
//////////////////////////////////////////////////////////////////////////////////

#include "stm32f10x.h"
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "lcd_init.h"
#include "lcd.h"
#include "ldc1614.h"
#include "key.h"

// ==================== 参数配置 ====================
#define AVG_COUNT   4          // 滑动平均窗口大小
#define UPDATE_MS   150        // 刷新周期(ms)
#define CHANGE_THRESHOLD 50    // 变化阈值
#define VALID_TOLERANCE  5     // 合格范围: ±5% (45%-55%)

// ==================== 工作模式 ====================
typedef enum {
    MODE_MEASURE = 0,    // 测量模式
    MODE_CAL_EMPTY,      // 空载校准模式
    MODE_CAL_VALID       // 合格件校准模式
} WorkMode_t;

// ==================== 全局变量 ====================
static u32 ldc_raw_buf[AVG_COUNT];  // 滤波缓冲区
static u8 ldc_buf_idx = 0;          // 缓冲区索引
static u32 ldc_last_display = 0;    // 上次显示值

static u32 empty_value = 0;         // 空载值(无金属,远端)
static u32 valid_value = 0;         // 合格件值(标准距离,中心点)
static u8 calibrated = 0;           // 校准状态: bit0=空载, bit1=合格件

static WorkMode_t current_mode = MODE_MEASURE;
static u8 welcome_shown = 0;        // 欢迎界面显示标志
static u8 need_refresh = 1;         // LCD刷新标志

// ==================== 函数声明 ====================
void System_Init(void);
u8 Sensor_Init(void);
u32 Sensor_ReadFiltered(void);
u16 Calculate_Distance(u32 raw_value);
void Display_UI(u16 distance, u32 raw_value);
void Display_CalibrationUI(WorkMode_t mode, u32 value);
void Display_WelcomeUI(void);
void Display_WaitingUI(void);
void Handle_KeyPress(u8 key);
void Calibrate_Empty(void);
void Calibrate_Valid(void);

// ==================== 主函数 ====================
/*
 *函数名: main
 *功能: 主程序入口,协调各模块实现金属距离检测
 *使用方法: 
 *  - SW2(WK_UP): 测量模式 / 确认校准
 *  - SW3(PC9):   合格件校准(放置标准件)
 *  - SW4(PC8):   空载校准(移除金属)
 *作者: 胡荣杰
 *时间: 2025年12月17日
 */
int main(void)
{
    u32 raw_value;
    u16 distance;
    u8 key;
    u16 welcome_counter;
    
    /* 系统初始化 */
    System_Init();
    
    /* 传感器初始化 */
    if (Sensor_Init() != 0)
    {
        while(1) 
        { 
            LED = !LED; 
            delay_ms(200); 
        }
    }
    
    printf("\n=== Valid Part Calibration System ===\r\n");
    printf("SW2: Measurement / Confirm\r\n");
    printf("SW3: Valid Part Cal\r\n");
    printf("SW4: Empty Cal\r\n");
    printf("Valid Range: 45%% - 55%%\r\n\r\n");
    
    welcome_counter = 0;
    
    /* 主循环 */
    while (1)
    {
        /* 检测按键 */
        key = Key_GetNum();
        if (key != KEY_NONE)
        {
            welcome_shown = 1;
            need_refresh = 1;  // 按键后需要刷新
            Handle_KeyPress(key);
        }
        
        /* 读取传感器数据 */
        raw_value = Sensor_ReadFiltered();
        
        /* 根据当前模式显示 */
        if (current_mode == MODE_MEASURE)
        {
            if ((calibrated & 0x03) == 0x03)  // 两点都已校准
            {
                /* 测量模式 - 持续刷新 */
                distance = Calculate_Distance(raw_value);
                Display_UI(distance, raw_value);
                
                printf("Raw:%lu Dist:%d%% E:%lu V:%lu\r\n", 
                       raw_value, distance, empty_value, valid_value);
            }
            else
            {
                /* 未完成校准 */
                if (!welcome_shown && welcome_counter < 50)
                {
                    /* 欢迎界面(5秒,只刷新一次) */
                    if (need_refresh)
                    {
                        Display_WelcomeUI();
                        need_refresh = 0;
                    }
                    welcome_counter++;
                }
                else
                {
                    welcome_shown = 1;
                    
                    /* 等待校准界面(只刷新一次) */
                    if (need_refresh)
                    {
                        Display_WaitingUI();
                        need_refresh = 0;
                    }
                }
            }
        }
        else if (current_mode == MODE_CAL_EMPTY)
        {
            /* 空载校准模式 - 持续刷新 */
            Display_CalibrationUI(MODE_CAL_EMPTY, raw_value);
        }
        else if (current_mode == MODE_CAL_VALID)
        {
            /* 合格件校准模式 - 持续刷新 */
            Display_CalibrationUI(MODE_CAL_VALID, raw_value);
        }
        
        /* LED心跳 */
        LED = !LED;
        
        /* 延时 */
        delay_ms(300);
    }
}

// ==================== 功能函数实现 ====================

/*
 *函数名: System_Init
 *功能: 初始化系统外设(延时/串口/LED/LCD/按键)
 *使用方法: 在main函数开始时调用一次
 *作者: 胡荣杰
 *时间: 2025年12月17日
 */
void System_Init(void)
{
    delay_init();
    uart_init(115200);
    LED_Init();
    Key_Init();
    
    printf("\r\n========================================\r\n");
    printf("  LDC1614 Distance Detector v4.5\r\n");
    printf("  Valid Part Calibration System\r\n");
    printf("  Author: Hu Rongjie\r\n");
    printf("  Date: 2025-12-17\r\n");
    printf("========================================\r\n");
    
    LCD_Init();
    delay_ms(50);
    
    /* 开机标题(3秒) */
    LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
    LCD_ShowString(20, 10, (u8*)"Inductive", BLACK, WHITE, 16, 0);
    LCD_ShowString(25, 30, (u8*)"Distance", BLACK, WHITE, 16, 0);
    LCD_ShowString(15, 50, (u8*)"Measurement", BLACK, WHITE, 16, 0);
		LCD_ShowString(15, 85, (u8*)"SWU XUEXING", DARKBLUE, WHITE, 16, 0);
		LCD_ShowString(25, 105, (u8*)"Group 3", DARKBLUE, WHITE, 16, 0);
    delay_ms(5000);
}

/*
 *函数名: Sensor_Init
 *功能: 初始化LDC1614传感器并预热滤波器
 *使用方法: 在System_Init之后调用,返回0=成功,1=失败
 *作者: 胡荣杰
 *时间: 2025年12月17日
 */
u8 Sensor_Init(void)
{
    u8 i;
    u8 init_status;
    
    /* 初始化界面(至少2秒) */
    LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
    LCD_ShowString(25, 55, (u8*)"Init...", BLACK, WHITE, 16, 0);
    delay_ms(500);
    
    init_status = LDC1614_init();
    
    if (init_status != 0)
    {
        printf("Sensor Failed!\r\n");
        LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
        LCD_ShowString(30, 55, (u8*)"FAIL", RED, WHITE, 16, 0);
        delay_ms(2000);
        return 1;
    }
    
    printf("Sensor OK!\r\n");
    
    /* 预热滤波器 */
    for (i = 0; i < AVG_COUNT; i++)
    {
        ldc_raw_buf[i] = LDC_get_channel_result(0);
        delay_ms(50);
    }
    ldc_last_display = ldc_raw_buf[0];
    
    /* 初始化完成 */
    LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
    LCD_ShowString(40, 55, (u8*)"Ready", GREEN, WHITE, 16, 0);
    delay_ms(1500);
    
    return 0;
}

/*
 *函数名: Sensor_ReadFiltered
 *功能: 读取传感器数据并进行滑动平均滤波
 *使用方法: 在主循环中周期性调用,返回滤波后的数值
 *作者: 胡荣杰
 *时间: 2025年12月17日
 */
u32 Sensor_ReadFiltered(void)
{
    u32 raw_current;
    u32 raw_avg;
    unsigned long sum;
    s32 change;
    u8 i;
    
    raw_current = LDC_get_channel_result(0);
    
    ldc_raw_buf[ldc_buf_idx] = raw_current;
    ldc_buf_idx = (ldc_buf_idx + 1) % AVG_COUNT;
    
    sum = 0;
    for (i = 0; i < AVG_COUNT; i++)
    {
        sum += ldc_raw_buf[i];
    }
    raw_avg = sum / AVG_COUNT;
    
    change = (s32)raw_avg - (s32)ldc_last_display;
    if (change < 0) change = -change;
    
    if (change > CHANGE_THRESHOLD)
    {
        ldc_last_display = raw_avg;
    }
    
    return ldc_last_display;
}

/*
 *函数名: Calculate_Distance
 *功能: 使用两点校准法计算距离百分比,合格件设为50%中心点
 *使用方法: 传入原始数值,返回距离百分比(100=空载,50=合格,0=近端)
 *作者: 胡荣杰
 *时间: 2025年12月17日
 */
u16 Calculate_Distance(u32 raw_value)
{
    s32 delta_empty_valid;
    s32 delta_current;
    s32 distance_signed;
    u16 distance;
    
    delta_empty_valid = (s32)valid_value - (s32)empty_value;
    if (delta_empty_valid == 0) return 50;
    
    delta_current = (s32)raw_value - (s32)empty_value;
    
    /* 合格件设为50%,线性外推到0% */
    distance_signed = 100 - ((delta_current * 100) / (delta_empty_valid * 2));
    
    /* 限幅 */
    if (distance_signed > 100) 
        distance = 100;
    else if (distance_signed < 0) 
        distance = 0;
    else 
        distance = (u16)distance_signed;
    
    return distance;
}

/*
 *函数名: Display_UI
 *功能: 测量模式下显示距离/原始值/进度条/状态
 *使用方法: 传入距离百分比和原始值,在主循环中调用
 *作者: 胡荣杰
 *时间: 2025年12月17日
 */
void Display_UI(u16 distance, u32 raw_value)
{
    u16 bar_len;
    
    LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
		LCD_ShowString(28, 5, (u8*)"Measuring", RED, YELLOW, 16, 0);
    
    /* 距离百分比 */
    LCD_ShowString(10, 30, (u8*)"Dist:", BLACK, WHITE, 16, 0);
		if (distance >=50){
			LCD_ShowString(50, 30, (u8*)"+", BLUE, WHITE, 16, 0);
			LCD_ShowIntNum(60, 30, distance - 50, 3, BLUE, WHITE, 16);
		}
		if (distance < 50){
			LCD_ShowString(50, 30, (u8*)"-", BLUE, WHITE, 16, 0);
			LCD_ShowIntNum(60, 30, 50 - distance, 3, BLUE, WHITE, 16);
		}
    LCD_ShowString(100, 30, (u8*)"%", BLUE, WHITE, 16, 0);
    
    /* 原始值 */
    LCD_ShowString(10, 55, (u8*)"Raw:", BLACK, WHITE, 16, 0);
    LCD_ShowIntNum(55, 55, raw_value / 1000, 5, BLACK, WHITE, 16);
    LCD_ShowString(100, 55, (u8*)"K", BLACK, WHITE, 16, 0);
    
    /* 进度条 */
    bar_len = distance;
    if (bar_len > 110) bar_len = 110;
    LCD_DrawRectangle(10, 90, 118, 105, BLACK);
    if (bar_len > 0)
    {
        LCD_Fill(11, 91, 10 + bar_len, 104, GREEN);
    }
    
    /* 状态判定: 45%-55%为合格 */
    if (distance >= 45 && distance <= 55)
    {
        LCD_ShowString(47, 110, (u8*)"PASS", GREEN, WHITE, 16, 0);
    }
    else if (distance > 55)
    {
        LCD_ShowString(50, 110, (u8*)"FAR ", BLUE, WHITE, 16, 0);
    }
    else
    {
        LCD_ShowString(47, 110, (u8*)"NEAR", RED, WHITE, 16, 0);
    }
}

/*
 *函数名: Display_CalibrationUI
 *功能: 校准模式下显示实时数据
 *使用方法: 传入校准模式和当前数值,在校准时调用
 *作者: 胡荣杰
 *时间: 2025年12月17日
 */
void Display_CalibrationUI(WorkMode_t mode, u32 value)
{
    LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
    
    if (mode == MODE_CAL_EMPTY)
    {
        LCD_ShowString(28, 5, (u8*)"Empty Cal", RED, YELLOW, 16, 0);
        LCD_ShowString(5, 30, (u8*)"Kepp Empty", BLACK, WHITE, 16, 0);
    }
    else
    {
        LCD_ShowString(28, 5, (u8*)"Valid Cal", RED, YELLOW, 16, 0);
        LCD_ShowString(5, 30, (u8*)"Put Std", BLACK, WHITE, 16, 0);
    }
    
    LCD_ShowString(5, 60, (u8*)"Value:", BLACK, WHITE, 16, 0);
    LCD_ShowIntNum(5, 80, value / 1000, 5, BLUE, WHITE, 16);
    LCD_ShowString(80, 80, (u8*)"K", BLUE, WHITE, 16, 0);
    
    LCD_ShowString(5, 105, (u8*)"K2:Confirm", GREEN, WHITE, 16, 0);
}

/*
 *函数名: Display_WelcomeUI
 *功能: 显示欢迎界面和操作指引(开机5秒)
 *使用方法: 系统启动后自动显示
 *作者: 胡荣杰
 *时间: 2025年12月17日
 */
void Display_WelcomeUI(void)
{
    LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
    
    LCD_ShowString(25, 10, (u8*)"Check List", BLUE, YELLOW, 16, 0);
		LCD_ShowString(5, 40, (u8*)"K3:Empty", RED, WHITE, 16, 0);
    LCD_ShowString(5, 70, (u8*)"K4:Valid", RED, WHITE, 16, 0);
    LCD_ShowString(5, 100, (u8*)"K2:Confirm", BLACK, WHITE, 16, 0);
}

/*
 *函数名: Display_WaitingUI
 *功能: 显示等待校准界面(静态显示,不刷新)
 *使用方法: 欢迎界面结束后,未完成校准时显示
 *作者: 胡荣杰
 *时间: 2025年12月17日
 */
void Display_WaitingUI(void)
{
    LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
	
		LCD_ShowString(25, 10, (u8*)"Check List", BLUE, YELLOW, 16, 0);
    
    if (!(calibrated & 0x01))
    {
        LCD_ShowString(5, 40, (u8*)"K3:Empty", RED, WHITE, 16, 0);
    }
    else
    {
        LCD_ShowString(5, 40, (u8*)"Empty OK", GREEN, WHITE, 16, 0);
    }
    
    if (!(calibrated & 0x02))
    {
        LCD_ShowString(5, 70, (u8*)"K4:Valid", RED, WHITE, 16, 0);
    }
    else
    {
        LCD_ShowString(5, 70, (u8*)"Valid OK", GREEN, WHITE, 16, 0);
    }
    
    LCD_ShowString(5, 100, (u8*)"K2:Confirm", BLACK, WHITE, 16, 0);
}

/*
 *函数名: Handle_KeyPress
 *功能: 处理按键事件,切换工作模式
 *使用方法: 在主循环中检测到按键后调用
 *作者: 胡荣杰
 *时间: 2025年12月17日
 */
void Handle_KeyPress(u8 key)
{
    if (key == KEY_WKUP)  // SW2
    {
        if (current_mode == MODE_CAL_EMPTY)
        {
            Calibrate_Empty();
            current_mode = MODE_MEASURE;
        }
        else if (current_mode == MODE_CAL_VALID)
        {
            Calibrate_Valid();
            current_mode = MODE_MEASURE;
        }
        else
        {
            current_mode = MODE_MEASURE;
        }
    }
    else if (key == KEY_SW3)  // SW3 - 合格件校准
    {
        current_mode = MODE_CAL_VALID;
        printf("\n>>> Valid Calibration <<<\r\n");
    }
    else if (key == KEY_SW4)  // SW4 - 空载校准
    {
        current_mode = MODE_CAL_EMPTY;
        printf("\n>>> Empty Calibration <<<\r\n");
    }
}

/*
 *函数名: Calibrate_Empty
 *功能: 执行空载校准,采集10次取平均作为空载基准值
 *使用方法: 移除所有金属后,在校准模式按K2确认时调用
 *作者: 胡荣杰
 *时间: 2025年12月17日
 */
void Calibrate_Empty(void)
{
    u8 i;
    u32 raw_current;
    unsigned long sum;
    
    sum = 0;
    for (i = 0; i < 10; i++)
    {
        raw_current = LDC_get_channel_result(0);
        sum += raw_current;
        delay_ms(50);
    }
    empty_value = sum / 10;
    calibrated |= 0x01;
    
    printf("Empty OK! Value: %lu\r\n", empty_value);
    
    LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
    LCD_ShowString(20, 55, (u8*)"Empty OK!", GREEN, WHITE, 16, 0);
    delay_ms(1000);
    
    need_refresh = 1;  // 需要刷新界面
}

/*
 *函数名: Calibrate_Valid
 *功能: 执行合格件校准,采集10次取平均作为50%中心标准值
 *使用方法: 放置标准工件后,在校准模式按K2确认时调用
 *作者: 胡荣杰
 *时间: 2025年12月17日
 */
void Calibrate_Valid(void)
{
    u8 i;
    u32 raw_current;
    unsigned long sum;
    
    sum = 0;
    for (i = 0; i < 10; i++)
    {
        raw_current = LDC_get_channel_result(0);
        sum += raw_current;
        delay_ms(50);
    }
    valid_value = sum / 10;
    calibrated |= 0x02;
    
    printf("Valid OK! Value: %lu\r\n", valid_value);
    
    LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
    LCD_ShowString(20, 55, (u8*)"Valid OK!", GREEN, WHITE, 16, 0);
    delay_ms(1000);
    
    need_refresh = 1;  // 需要刷新界面
}
