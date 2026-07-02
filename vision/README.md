# Vision — Ball Tracking (PC)

The PC-side computer-vision pipeline. It detects the ball in a webcam stream and streams its pixel coordinates to the STM32 over a USB serial (virtual COM) link.

## Files

| File | Purpose |
|------|---------|
| `ball_tracker.py` | Main program. Detects the ball, draws overlays, and sends `"x,y\n"` over serial. |
| `circle_calibration.py` | Standalone helper that draws a circle + center dot on the camera feed so you can physically align the camera with the plate. Run this first. |
| `requirements.txt` | Python dependencies. |

## Setup

```bash
pip install -r requirements.txt
python ball_tracker.py
```

Press **`q`** or **`Esc`** to quit. A live overlay shows the serial connection state (`STM: Connected/Disconnected`) and the detected ball coordinates.

## Configuration

Edit the constants at the top of `ball_tracker.py`:

| Constant | Default | Meaning |
|----------|---------|---------|
| `serial_port` | `"COM11"` | Serial port of the STM32 virtual COM. Linux/macOS: `/dev/ttyACM0`, `/dev/tty.usbmodemXXXX`. |
| `baud_rate` | `115200` | Must match the firmware. |
| `min_r`, `max_r` | `50`, `70` | Hough circle radius band (pixels). Tune to your ball size & camera distance. |

The detection also has tunables inside `find_white_ball_center()`:
- `cv2.HoughCircles` parameters (`dp`, `minDist`, `param1`, `param2`)
- the brightness gate (`mean_inside > 200`) that distinguishes the white ball from other circles.

## How detection works

1. Convert frame to grayscale, blur with a 9×9 Gaussian.
2. `cv2.HoughCircles` finds circular candidates within the radius band.
3. For each candidate, the mean brightness *inside* the circle is measured; only bright (white-ball) circles pass.
4. The accepted circle's center `(x, y)` is sent as `"x,y\n"`.
5. A fixed **reference circle** is drawn at `(320, 240)` every frame to mark the target center for alignment.

> **Colored-ball variant:** the project also explored an HSV color-threshold approach (isolating an orange ball) before the circle transform. See the slide deck in `docs/presentation/` for that pipeline's parameters.

## Protocol

Plain ASCII, newline-terminated, one message per detection:

```
320,240\n
318,235\n
...
```

The firmware parses both `"x,y"` and `"(x,y)"` forms.
