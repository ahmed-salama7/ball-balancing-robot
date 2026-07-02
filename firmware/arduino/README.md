# Firmware — Arduino (Reference Prototype)

This is the **reference implementation** of the control loop, written for the Arduino framework. It is the clearest way to read and understand the algorithm (serial parsing → PID → inverse kinematics → servo output). The deployed build runs on the STM32 (see [`../stm32/`](../stm32/)).

## Files

| File | Purpose |
|------|---------|
| `ball_balancer.ino` | Main sketch: serial RX, PID controller, IK call, servo mapping, 50 Hz loop. |
| `InverseKinematics.cpp` | The `Machine` class — closed-form IK for the 3-RRS platform. |
| `InverseKinematics.h` | Class/constants declaration. |

> Arduino requires the main sketch and its folder to share a name, so `main.ino` was renamed to `ball_balancer.ino` inside the `ball_balancer/` folder. The IK files sit alongside it in the same sketch folder.

## Build & upload

1. Open `ball_balancer/ball_balancer.ino` in the Arduino IDE.
2. Install the **Servo** library (Library Manager).
3. Select your board and port, then upload.

## What the code does

```
loop():
  read serial bytes → assemble a line → parse "x,y"
  PID_compute(x, y)              # error = center - ball, discrete PID per axis
  moveTo(platformHz, -out_x, -out_y)
      → machine.theta(...) for legs A, B, C   # inverse kinematics
      → map θ to servo angle, constrain to safe travel
      → servo.write(...)
  (failsafe: if no fresh data for ~500 ms, command the plate flat)
```

## Key parameters (top of `ball_balancer.ino`)

| Parameter | Meaning |
|-----------|---------|
| `Machine machine(5.7, 8.5, 3.1, 9.66)` | IK geometry `(d, e, f, g)` in cm. |
| `servoPinA/B/C` (`9`, `10`, `11`) | Servo signal pins (Arduino framework numbering). |
| `servoNeutralA/B/C` | Per-servo neutral angle for a level plate. |
| `servoMin`, `servoMax` | Safe travel limits. |
| `kp`, `ki`, `kd` | PID gains (commented alternatives included). |
| `platformHz` | Baseline platform height (cm) for the IK. |
| `maxSlope` | Tilt-slope saturation for safety. |
| `LOOP_PERIOD_MS` | Control period (20 ms → 50 Hz). |
| `centerX`, `centerY` | Target pixel center (`320`, `240`). |

> **Pin numbering differs by target.** This sketch uses Arduino pins `9/10/11`. The STM32 HAL firmware uses hardware-timer channels on **PA0 / PA1 / PA3**. Both drive the same three servos — see the main [README pinout](../../README.md#stm32-pinout).
