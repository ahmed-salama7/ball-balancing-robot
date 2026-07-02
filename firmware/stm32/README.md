# Firmware — STM32 (Deployed)

The deployed firmware for the **STM32F103C8T6 "Blue Pill"**, built with **STM32CubeIDE** (HAL). It receives ball coordinates over a USB CDC virtual COM port, runs the PID + inverse-kinematics control loop, and drives the three servos with hardware PWM on TIM2.

## ⚠️ Read this first — completing the project

This folder contains the **CubeIDE project files** but **not** the generated source/driver folders (they are large and machine-generated). Before the project will build, you need:

```
firmware/stm32/
├── Core/            ←  ADD: Inc/ + Src/ (main.c, stm32f1xx_it.c, msp, system_*)
├── Drivers/         ←  ADD: CMSIS + STM32F1xx_HAL_Driver
├── Middlewares/     ←  ADD: ST USB Device Library (Core + CDC class)
├── USB_DEVICE/      ←  ADD: App/ + Target/ (usb_device.c, usbd_cdc_if.c, usbd_desc.c, usbd_conf.c)
│
├── usb_cdc.ioc              ✓ included (CubeMX configuration)
├── STM32F103C8TX_FLASH.ld   ✓ included (linker script)
├── .cproject / .project / .mxproject   ✓ included
└── usb_cdc Debug.launch     ✓ included (ST-Link debug config)
```

**Two ways to get the missing folders:**

- **Option A — copy from your local project.** If you still have the original `usb_cdc/` project, copy its `Core/`, `Drivers/`, `Middlewares/`, and `USB_DEVICE/` folders into this directory. (Your control logic lives in `Core/Src/main.c` and the USB receive callback in `USB_DEVICE/App/usbd_cdc_if.c`.) This is the recommended path because it preserves your hand-written code.

- **Option B — regenerate the skeleton from the `.ioc`.** Open `usb_cdc.ioc` in STM32CubeIDE and generate code. This recreates the HAL/USB skeleton, **but you must re-add your control code** (PID, IK, serial parsing) into the `/* USER CODE */` sections — regeneration only produces the empty framework.

> **Tip:** the Arduino sketch in [`../arduino/`](../arduino/) contains the full reference control logic (PID + IK) you can port into `main.c`.

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
2. Ensure the source folders above are present (Option A or B).
3. **Build** (hammer icon).
4. Connect **ST-Link V2** to the SWD header, then **Run/Debug** (uses `usb_cdc Debug.launch`).
5. After flashing, plug the Blue Pill's USB into the PC — it enumerates as a **Virtual COM Port**. Put that port name in `vision/ball_tracker.py`.

## Notes

- Servos draw far more current than the Blue Pill's regulator can supply — power them from the **external 5 V adapter**, sharing a common ground.
- The CDC receive callback (`CDC_Receive_FS` in `usbd_cdc_if.c`) is where incoming `"x,y\n"` bytes are handled and fed to the controller.
