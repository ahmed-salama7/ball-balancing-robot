# Firmware — STM32 (Deployed)

The deployed firmware for the **STM32F103C8T6 "Blue Pill"**, built with **STM32CubeIDE** (HAL). It receives ball coordinates over a USB CDC virtual COM port, runs the PID + inverse-kinematics control loop, and drives the three servos with hardware PWM on TIM2.

## Project layout

This is the **complete** CubeIDE project — open it and build. The main pieces:

| Path | Contents |
|------|----------|
| `Core/` | Application code — `Src/main.c` (control loop) plus HAL init and interrupt handlers. |
| `Drivers/` | CMSIS + STM32F1xx HAL drivers. |
| `Middlewares/` | ST USB Device library. |
| `USB_DEVICE/` | USB CDC (virtual COM) app layer. |
| `usb_cdc.ioc` | CubeMX configuration (TIM2 PWM + USB CDC) — open to review peripherals. |
| `STM32F103C8TX_FLASH.ld` | Linker script. |
| `.cproject` / `.project` / `.mxproject` | CubeIDE project files. |

> **Where the logic lives:** the PID + inverse-kinematics control loop is in `Core/Src/main.c`, and the callback that parses incoming `"x,y\n"` coordinates is `CDC_Receive_FS` in `USB_DEVICE/App/usbd_cdc_if.c`. The same reference algorithm is mirrored in the Arduino sketch under [`../arduino/`](../arduino/).
>
> If you ever need to regenerate the HAL/USB skeleton, open `usb_cdc.ioc` in CubeMX and generate code — edits kept inside the `/* USER CODE */` blocks are preserved.

## Configuration summary (from `usb_cdc.ioc`)

| Setting | Value |
|---------|-------|
| MCU | STM32F103C8T6 (LQFP48) |
| System clock | 72 MHz (HSE 8 MHz crystal × PLL9) |
| USB clock | 48 MHz |
| **TIM2** | PWM, `Prescaler = 72−1`, `Period = 20000−1` → **50 Hz**, 1 µs resolution |
| TIM2_CH1 → **PA0** | Servo A PWM |
| TIM2_CH2 → **PA1** | Servo B PWM |
| TIM2_CH4 → **PA3** | Servo C PWM |
| **USB** | Device, **CDC** class (Virtual COM Port, "STM32 Virtual Port") |
| USB_DM / USB_DP | PA11 / PA12 |
| SWD | PA13 (SWDIO) / PA14 (SWCLK) |
| PC13 | GPIO output (on-board LED) |

## Build & flash

1. **STM32CubeIDE** → *File ▸ Open Projects from File System…* → select this `stm32/` folder.
2. **Build** (hammer icon).
3. Connect **ST-Link V2** to the SWD header, then **Run/Debug** (uses `usb_cdc Debug.launch`).
4. After flashing, plug the Blue Pill's USB into the PC — it enumerates as a **Virtual COM Port**. Put that port name in `vision/ball_tracker.py`.

## Notes

- Servos draw far more current than the Blue Pill's regulator can supply — power them from the **external 5 V adapter**, sharing a common ground.
- The CDC receive callback (`CDC_Receive_FS` in `usbd_cdc_if.c`) is where incoming `"x,y\n"` bytes are handled and fed to the controller.
