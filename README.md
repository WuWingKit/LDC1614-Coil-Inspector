# LDC1614 Coil Inspector

![Physical prototype of the LDC1614 coil inspector](./assets/product-photo.jpg)

[中文](./README.zh-CN.md) · [Demo video](./media/demonstration.mp4) · [Presentation](./presentation/project-presentation.pptx) · [Project proposal](./docs/project-proposal.docx)

[![STM32F103](https://img.shields.io/badge/MCU-STM32F103RC-03234B?logo=stmicroelectronics)](https://www.st.com/en/microcontrollers-microprocessors/stm32f103rc.html)
[![LDC1614](https://img.shields.io/badge/Sensor-LDC1614-CC0000)](https://www.ti.com/product/LDC1614)
[![C](https://img.shields.io/badge/Language-C-A8B9CC?logo=c)](https://www.iso.org/standard/82075.html)
[![Keil](https://img.shields.io/badge/IDE-Keil_MDK-394049)](https://www.keil.com/)
[![License: CC BY-NC-SA 4.0](https://img.shields.io/badge/Archive-CC_BY--NC--SA_4.0-lightgrey.svg)](./LICENSE-CONTENT.md)

An STM32-based, non-contact inductive inspection device for judging whether a metal workpiece is positioned or tightened within a qualified range. The photograph above shows the completed prototype with its 3D-printed enclosure, LCD, control keys, sensing coil, and workpiece fixture.

## Project at a glance

| Item | Details |
|---|---|
| Project period | **November–December 2025** |
| Course | Electronic Circuits Project |
| Project lead | **Hu Rongjie (胡荣杰)** |
| Actual developer | **Hu Rongjie — sole designer and developer** |
| Nominal presentation group | Hu Rongjie, Tang Lisa, Zhang Runhan, Shi Zhoulu, Dong Jiahui |
| Contribution clarification | The additional names above appeared on the course presentation only. The circuit/system design, firmware, enclosure integration, testing, debugging, and project documentation were completed by **Hu Rongjie alone**. |
| Project type | Embedded hardware prototype + firmware + 3D-printed enclosure |
| Prototype cost | **Not recorded in the supplied materials — to be added** |

## Commercialization and application analysis

The project targets a narrow but practical gap between manual visual checks and expensive industrial machine-vision stations. An inductive probe is insensitive to ambient light and remains usable around dust or oil, making it suitable for detecting metal presence, insertion depth, nut tightening position, or fixture alignment.

Potential customers include small assembly shops, teaching laboratories, fixture builders, and low-volume production lines. A commercial version would need a more rigid mechanical datum, interchangeable probe fixtures, traceable calibration, statistical quality logging, EMC testing, and a protected industrial power/interface stage. The strongest product path is therefore a low-cost, task-specific inspection fixture rather than a general-purpose precision displacement meter.

## How it works

The sensing coil and its capacitor form an LC resonant circuit. A nearby conductive target produces eddy currents, changing the coil's effective inductance and loss. The **LDC1614** converts this change into a high-resolution digital measurement; the **STM32F103RC** filters the readings, maps them to a relative position, performs the pass/fail decision, and updates the color LCD.

The physical prototype combines purchased electronic/mechanical modules with a custom **3D-printed enclosure** containing the display area, measurement platform, buttons, and internal storage space.

![3D-printed enclosure design](./assets/enclosure-design.png)

## System architecture

```text
Metal target
    ↓ changes coil inductance and resonant response
LC sensing coil → LDC1614 channel 0
                    ↓ 28-bit conversion result over software I²C
                STM32F103RC
                    ├─ 4-sample moving-average filter
                    ├─ two-point calibration and 0–100% mapping
                    ├─ PASS / FAR / NEAR decision
                    ├─ LCD interface and progress bar
                    └─ USART diagnostic output
```

### Firmware layers

| Layer | Main paths | Responsibility |
|---|---|---|
| Application | `USER/main.c` | Operating modes, key events, calibration, filtering, decision logic, LCD pages, and serial diagnostics |
| Sensor driver | `HARDWARE/LDC1614/` | Software I²C, register access, LDC1614 initialization, and channel conversion reads |
| Human interface | `HARDWARE/LCD/`, `HARDWARE/KEY/`, `HARDWARE/LED/` | Local display, keys, and status indication |
| Platform support | `SYSTEM/`, `STM32F10x_FWLIB/`, `CORE/` | Clock, delay, USART, CMSIS, startup, and STM32 Standard Peripheral Library |

The main loop first scans the keys, then reads and filters the LDC1614 value. A small state machine selects the empty-calibration, qualified-reference calibration, waiting, or measurement page. Measurement mode runs only after both calibration bits are set.

### Key firmware parameters

| Parameter | Current value |
|---|---:|
| LDC1614 I²C address | `0x2A` |
| Software I²C pins | `PB10` SCL, `PB11` SDA |
| LDC shutdown pin | `PC13`, active low in the driver |
| Moving-average window | 4 samples |
| Display update period | 150 ms |
| Acceptance window | 45–55% |

### Calibration and decision logic

1. Record an empty-fixture reference.
2. Place a qualified reference workpiece and record the second point.
3. Map live readings to a relative `0–100%` scale.
4. Classify `45–55%` as **PASS**, above `55%` as **FAR**, and below `45%` as **NEAR**.

The source code confirms this two-point procedure and the `45–55%` acceptance window. An early proposal mentions the LDC1314; the implemented hardware and firmware use the **LDC1614**.

![Startup and interface states](./assets/startup-and-ui.png)

## Design targets

These values are targets documented in the final presentation, not metrology certification results:

| Metric | Target |
|---|---:|
| Display refresh | ≤ 300 ms |
| Relative resolution | ≤ 1% |
| Static fluctuation | ≤ ±2% |
| Repeatability error | ≤ ±3% |
| Calibration flow | ≤ 5 user steps |

## Repository contents

- `USER/Inductive distance measurement.uvprojx`: Keil MDK project entry point
- `USER/main.c`: application state machine, filter, calibration, decision, and display logic
- `HARDWARE/LDC1614/`: software I²C and LDC1614 register driver
- `HARDWARE/LCD/`, `KEY/`, `LED/`: local interface drivers
- `SYSTEM/`, `CORE/`, `STM32F10x_FWLIB/`: STM32 platform support and libraries
- `assets/`: README illustrations exported from the final presentation
- `docs/project-proposal.docx`: original project proposal
- `presentation/project-presentation.pptx`: original final presentation
- `media/demonstration.mp4`: prototype demonstration
- `OBJ/`: historical build outputs retained from the original code archive

## Download, build, and use

### 1. Download the source

```bash
git clone https://github.com/WuWingKit/LDC1614-Coil-Inspector.git
cd LDC1614-Coil-Inspector
```

Without Git, open the repository page, choose **Code → Download ZIP**, and extract the archive.

### 2. Prepare the toolchain

- Keil MDK 5 with an ARM Compiler version compatible with the existing project
- ST-Link and its USB driver
- STM32F103RC target board
- LDC1614 board and LC sensing coil
- LCD and keys wired according to the supplied firmware pin definitions

The driver intentionally uses `PB10/PB11` for software I²C because `PB8/PB9` conflict with the LCD connections in this prototype. Check voltage, ground, SDA/SCL pull-ups, and the LDC1614 address before powering the system.

### 3. Build and flash

1. Open `USER/Inductive distance measurement.uvprojx` in Keil MDK.
2. Select the existing target and run **Build** (`F7`).
3. Connect ST-Link through SWD and run **Download** (`F8`).
4. If the sensor startup page shows `FAIL`, inspect the I²C wiring, address selection, shutdown pin, and common ground.

### 4. Calibrate and measure

1. Start the empty calibration with no metal target on the fixture, then confirm the reading.
2. Place the qualified reference workpiece at its required position and save the second point.
3. Enter measurement mode only after the interface shows both calibration steps as complete.
4. Place a workpiece on the fixture and read the percentage, raw value, progress bar, and `PASS`/`FAR`/`NEAR` result.

Calibration values live in RAM and must be captured again after a reset. Use the serial output when checking unstable readings or unexpected classification.

## Current limitations

- The result is relative to the two calibration references, not an absolute distance in millimetres.
- Fixture rigidity, target material, target geometry, temperature, and coil alignment influence the reading.
- No long-term calibration traceability, production statistics, or industrial I/O is implemented.
- The supplied archive does not contain a verified bill of materials or cost record.

## License

Project-authored documentation, presentation, images, video, and hardware-design material are shared under **CC BY-NC-SA 4.0**; see [LICENSE-CONTENT.md](./LICENSE-CONTENT.md). Source and third-party vendor components retain the terms stated in their respective files.
