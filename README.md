# LDC1614 线圈装配智能检测系统

**基于电感变化原理的工件合格性检测 | Coil Assembly Inspection System Based on Inductance Sensing**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-STM32F103RC-blue.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32f103rc.html)
[![Sensor](https://img.shields.io/badge/Sensor-LDC1614-green.svg)](https://www.ti.com/product/LDC1614)
[![IDE](https://img.shields.io/badge/IDE-Keil%20MDK%205-orange.svg)](https://www.keil.com/)

---

## 📖 中文说明

### 项目简介

本项目利用 **TI LDC1614** 四通道电感数字转换器，通过检测 LC 谐振线圈周围金属目标引起的电感变化，判断工件线圈是否正常装配（Pass / Fail）。系统搭载 STM32F103RC 主控，配合 TFT LCD 实时显示检测结果，并通过 UART 串口输出调试数据。

### 主要特性

- **高精度电感检测**：LDC1614 提供最高 28-bit 分辨率的电感测量
- **四通道扫描**：支持 CH0–CH3 独立通道，可扩展至多线圈并行检测
- **自动基准校准**：上电后自动采集基准值，消除环境漂移影响
- **滑动平均滤波**：4 点滑动平均，有效抑制噪声干扰
- **LCD 实时显示**：距离百分比、原始值、进度条、状态（FAR / MID / NEAR）
- **UART 调试输出**：波特率 115200，实时输出 Base / Raw / Delta / Dist 数据
- **LED 心跳指示**：主循环运行状态可视化

### 硬件连接

| 信号     | STM32 引脚 | 说明             |
|----------|-----------|-----------------|
| I²C SCL  | PB10      | 软件模拟 I²C，开漏输出 |
| I²C SDA  | PB11      | 软件模拟 I²C，开漏输出 |
| ADDR     | PA11      | 低电平 → I²C 地址 0x2A |
| SD（使能）| PC13      | 低电平有效，正常运行 |
| LED      | （见 led.h）| 心跳指示灯 |
| TFT LCD  | SPI/GPIO  | 见 lcd_init.h 引脚定义 |

### 软件架构

```
USER/
  main.c              ← 主逻辑：初始化、校准、主循环
HARDWARE/
  LDC1614/
    LDC1614.c/h       ← LDC1614 驱动（软件 I²C + 寄存器配置）
  LCD/
    lcd.c/h           ← LCD 显示驱动
    lcd_init.c/h      ← LCD 初始化
  LED/
    led.c/h           ← LED 驱动
SYSTEM/
  delay/              ← 延时函数
  usart/              ← 串口驱动
  sys/                ← 系统时钟
CORE/                 ← ARM Cortex-M3 内核文件
STM32F10x_FWLib/      ← ST 标准外设库
```

### 快速开始

1. **克隆仓库**
   ```bash
   git clone https://github.com/YOUR_USERNAME/LDC1614-Coil-Inspector.git
   ```

2. **打开工程**  
   使用 **Keil MDK 5** 打开 `USER/Inductive distance measurement.uvprojx`

3. **编译与烧录**  
   点击 Build（F7）编译，使用 ST-Link / J-Link 下载固件到 STM32F103RC

4. **串口监控**（可选）  
   打开任意串口助手，波特率 `115200`，可实时查看传感器数据

### 检测原理

```
线圈正常装配 → 金属工件处于标准位置 → 电感值稳定在基准范围内 → PASS
线圈缺失/错位 → 电感值偏离基准 → Delta 超过阈值 → FAIL
```

距离百分比计算：`distance = 100 - (delta × 100 / MAX_DELTA)`
- `distance > 80`：FAR（工件偏远/未装配）
- `20 ≤ distance ≤ 80`：MID（正常范围）
- `distance < 20`：NEAR（工件过近）

### 开发环境

| 项目 | 版本 |
|------|------|
| IDE  | Keil MDK 5 |
| 编译器 | ARM Compiler v5 (ARMCC) |
| 目标芯片 | STM32F103RC（HD 系列） |
| 标准库 | STM32F10x StdPeriph Library v3.5 |
| 晶振 | 外部 HSE（见工程配置） |

---

## 📖 English Documentation

### Project Overview

This project uses the **TI LDC1614** 4-channel inductance-to-digital converter to detect whether a coil is properly assembled on a workpiece. The system measures the inductance shift caused by a metallic target near the LC resonance coil and outputs a **Pass / Fail** judgment. The STM32F103RC microcontroller handles sensor communication, signal processing, and display output.

### Key Features

- **High-resolution inductive sensing**: LDC1614 offers up to 28-bit inductance measurement
- **4-channel support**: CH0–CH3 independent channels for multi-coil inspection
- **Auto baseline calibration**: Captures baseline at startup to compensate for environmental drift
- **Sliding average filter**: 4-sample moving average for noise suppression
- **Real-time LCD display**: Distance %, raw value, progress bar, and status (FAR / MID / NEAR)
- **UART debug output**: 115200 baud, streams Base / Raw / Delta / Dist in real time
- **LED heartbeat**: Indicates main loop is running

### Hardware Connections

| Signal    | STM32 Pin | Notes                              |
|-----------|-----------|------------------------------------|
| I²C SCL   | PB10      | Bit-bang I²C, open-drain output    |
| I²C SDA   | PB11      | Bit-bang I²C, open-drain output    |
| ADDR      | PA11      | Low → I²C address 0x2A            |
| SD (EN)   | PC13      | Active-low, keep low for normal op |
| LED       | (see led.h)| Heartbeat indicator               |
| TFT LCD   | SPI/GPIO  | See lcd_init.h for pin mapping     |

### Software Architecture

```
USER/
  main.c              ← Main logic: init, calibration, main loop
HARDWARE/
  LDC1614/
    LDC1614.c/h       ← LDC1614 driver (bit-bang I²C + register config)
  LCD/
    lcd.c/h           ← TFT LCD display driver
    lcd_init.c/h      ← LCD initialization
  LED/
    led.c/h           ← LED driver
SYSTEM/
  delay/              ← Delay utilities
  usart/              ← UART driver
  sys/                ← System clock
CORE/                 ← ARM Cortex-M3 core files
STM32F10x_FWLib/      ← ST Standard Peripheral Library
```

### Getting Started

1. **Clone the repository**
   ```bash
   git clone https://github.com/YOUR_USERNAME/LDC1614-Coil-Inspector.git
   ```

2. **Open the project**  
   Open `USER/Inductive distance measurement.uvprojx` with **Keil MDK 5**

3. **Build and flash**  
   Press F7 to build, then flash to STM32F103RC via ST-Link or J-Link

4. **Monitor serial output** (optional)  
   Open any serial terminal at `115200` baud to view real-time sensor data

### Detection Principle

```
Coil properly assembled → Metal target at nominal position → Inductance within baseline range → PASS
Coil missing / misaligned → Inductance deviates from baseline → Delta exceeds threshold → FAIL
```

Distance percentage: `distance = 100 - (delta × 100 / MAX_DELTA)`
- `distance > 80`: FAR (target absent or far away)
- `20 ≤ distance ≤ 80`: MID (normal assembly range)
- `distance < 20`: NEAR (target too close)

### Development Environment

| Item       | Version / Details                      |
|------------|----------------------------------------|
| IDE        | Keil MDK 5                             |
| Compiler   | ARM Compiler v5 (ARMCC)                |
| Target MCU | STM32F103RC (High-Density)             |
| Library    | STM32F10x StdPeriph Library v3.5       |
| Clock      | External HSE (see project settings)    |

---

## 📁 Repository Structure

```
LDC1614-Coil-Inspector/
├── CORE/                          # ARM Cortex-M3 core files
│   ├── core_cm3.c / core_cm3.h
│   └── startup_stm32f10x_hd.s    # Startup file for HD series
├── HARDWARE/
│   ├── LDC1614/
│   │   ├── LDC1614.c             # Sensor driver (I²C + registers)
│   │   └── LDC1614.h
│   ├── LCD/                       # TFT LCD driver
│   └── LED/                       # LED driver
├── SYSTEM/
│   ├── delay/                     # Delay functions
│   ├── sys/                       # System utilities
│   └── usart/                     # UART driver
├── STM32F10x_FWLib/               # ST Standard Peripheral Library
├── USER/
│   ├── main.c                     # Main application
│   ├── Inductive distance measurement.uvprojx   # Keil project file
│   └── stm32f10x.h / stm32f10x_conf.h
├── .gitignore                     # Excludes build artifacts
├── LICENSE                        # MIT License
└── README.md                      # This file
```

---

## 🙏 Acknowledgements

- [TI LDC1614 Datasheet](https://www.ti.com/product/LDC1614)
- STM32F10x Standard Peripheral Library by STMicroelectronics
- LCD driver adapted from open-source community contributions

## 📄 License

This project is licensed under the **MIT License** — see [LICENSE](LICENSE) for details.  
本项目使用 **MIT 协议** 开源，详见 [LICENSE](LICENSE) 文件。
