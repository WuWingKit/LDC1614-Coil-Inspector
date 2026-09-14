# LDC1614 Coil Inspector

[中文](./README.zh-CN.md) · [Demo video](./media/demonstration.mp4) · [Presentation](./presentation/project-presentation.pptx) · [Project proposal](./docs/project-proposal.docx)

[![STM32F103](https://img.shields.io/badge/MCU-STM32F103RC-03234B?logo=stmicroelectronics)](https://www.st.com/en/microcontrollers-microprocessors/stm32f103rc.html)
[![LDC1614](https://img.shields.io/badge/Sensor-LDC1614-CC0000)](https://www.ti.com/product/LDC1614)
[![C](https://img.shields.io/badge/Language-C-A8B9CC?logo=c)](https://www.iso.org/standard/82075.html)
[![Keil](https://img.shields.io/badge/IDE-Keil_MDK-394049)](https://www.keil.com/)
[![License: CC BY-NC-SA 4.0](https://img.shields.io/badge/Archive-CC_BY--NC--SA_4.0-lightgrey.svg)](./LICENSE-CONTENT.md)

An STM32-based, non-contact inductive inspection device for judging whether a metal workpiece is positioned or tightened within a qualified range.

![3D-printed enclosure design](./assets/enclosure-design.png)

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

- `USER/` and `HARDWARE/`: STM32 application and peripheral drivers
- `assets/`: README illustrations exported from the final presentation
- `docs/project-proposal.docx`: original project proposal
- `presentation/project-presentation.pptx`: original final presentation
- `media/demonstration.mp4`: prototype demonstration
- `OBJ/`: historical build outputs retained from the original code archive

## Build and use

1. Open the Keil project under `USER/` in Keil MDK.
2. Build with the STM32F10x Standard Peripheral Library configuration already included in the repository.
3. Flash the STM32F103RC with an ST-Link.
4. Power on, complete empty and qualified-reference calibration, then place the target on the inspection platform.

## Current limitations

- The result is relative to the two calibration references, not an absolute distance in millimetres.
- Fixture rigidity, target material, target geometry, temperature, and coil alignment influence the reading.
- No long-term calibration traceability, production statistics, or industrial I/O is implemented.
- The supplied archive does not contain a verified bill of materials or cost record.

## License

Project-authored documentation, presentation, images, video, and hardware-design material are shared under **CC BY-NC-SA 4.0**; see [LICENSE-CONTENT.md](./LICENSE-CONTENT.md). Source and third-party vendor components retain the terms stated in their respective files.

