import cv2
import numpy as np
import serial
import time
import sys
import signal

# globals so signal handler can access them
cap = None
ser = None

def find_white_ball_center(frame, min_r=45, max_r=70):
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    blurred = cv2.GaussianBlur(gray, (9, 9), 0)
    circles = cv2.HoughCircles(
        blurred,
        method=cv2.HOUGH_GRADIENT,
        dp=1.2,
        minDist=100,
        param1=100,
        param2=30,
        minRadius=min_r,
        maxRadius=max_r
    )
    if circles is not None:
        circles = np.round(circles[0, :]).astype("int")
        for (x, y, r) in circles:
            mask = np.zeros_like(gray)
            cv2.circle(mask, (x, y), r, 255, thickness=-1)
            mean_inside = cv2.mean(gray, mask=mask)[0]
            if mean_inside > 200:
                return (x, y, r)
    return None

def cleanup(cap_obj, ser_obj):
    """Release camera, close windows, close serial (silent)."""
    try:
        if cap_obj is not None:
            cap_obj.release()
    except Exception:
        pass
    try:
        cv2.destroyAllWindows()
    except Exception:
        pass
    try:
        if ser_obj is not None and getattr(ser_obj, "is_open", False):
            ser_obj.close()
    except Exception:
        pass

def signal_handler(sig, frame):
    """Handle signals like Ctrl+C (SIGINT) silently."""
    cleanup(cap, ser)
    sys.exit(0)

if __name__ == "__main__":
    serial_port = "COM11"  # or your port, e.g. "COM3"
    baud_rate = 115200

    # Open serial (silent on failure)
    try:
        ser = serial.Serial(serial_port, baud_rate, timeout=1)
        time.sleep(2)
    except Exception:
        ser = None

    # Open camera
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        cleanup(cap, ser)
        sys.exit(1)

    # Register signal handler
    signal.signal(signal.SIGINT, signal_handler)

    try:
        # keep last known ball coords (or None)
        ball_coords = None

        while True:
            ret, frame = cap.read()
            if not ret:
                break

            res = find_white_ball_center(frame, min_r=50, max_r=70)
            if res is not None:
                x, y, r = res
                ball_coords = (int(x), int(y))

                # draw detection
                cv2.circle(frame, (int(x), int(y)), int(r), (0, 255, 0), 2)
                cv2.circle(frame, (int(x), int(y)), 2, (0, 0, 255), 3)

                # send as "x,y" (no parentheses), newline terminated
                if ser is not None and getattr(ser, "is_open", False):
                    data_str = f"{int(x)},{int(y)}"
                    try:
                        ser.write(data_str.encode('ascii') + b'\n')
                    except Exception:
                        # on write failure, silently ignore and mark ser closed
                        try:
                            ser.close()
                        except Exception:
                            pass
                        ser = None
            else:
                ball_coords = None

            # Draw constant circle with center (320,240) and radius 210 always
            # This will be drawn on every frame
            try:
                cv2.circle(frame, (320, 240), 195, (255, 0, 0), 2)
            except Exception:
                # in case frame is smaller or some unexpected error, ignore
                pass

            # Show STM connection status overlay and ball pixel coordinates
            connected = (ser is not None and getattr(ser, "is_open", False))
            status_text = "STM: Connected" if connected else "STM: Disconnected"
            if ball_coords is not None:
                ball_text = f"Ball: {ball_coords[0]},{ball_coords[1]}"
            else:
                ball_text = "Ball: Not detected"

            # prepare stacked text (two lines)
            lines = [status_text, ball_text]
            font = cv2.FONT_HERSHEY_SIMPLEX
            font_scale = 0.8
            thickness = 2
            padding_x = 6
            padding_y = 6
            line_spacing = 8  # px between lines

            # compute widths and heights for both lines
            sizes = [cv2.getTextSize(ln, font, font_scale, thickness)[0] for ln in lines]
            widths = [s[0] for s in sizes]
            heights = [s[1] for s in sizes]
            max_width = max(widths)
            total_height = sum(heights) + (len(lines) - 1) * line_spacing

            # top-left corner for rectangle background
            txt_pos = (10, 30)  # baseline of first line
            rect_x1 = txt_pos[0] - padding_x
            rect_y2 = txt_pos[1] + padding_y  # baseline + padding bottom for first line
            # compute top y for rectangle: move up by heights[0] and subsequent lines
            rect_y1 = txt_pos[1] - heights[0] - padding_y
            # extend rectangle for additional lines below first
            if len(lines) > 1:
                rect_y2 = rect_y2 + (total_height - heights[0])

            # ensure rectangle covers full width
            rect_x2 = txt_pos[0] + max_width + padding_x

            # draw background rect
            cv2.rectangle(frame, (rect_x1, rect_y1), (rect_x2, rect_y2), (0, 0, 0), -1)

            # draw each line
            y_offset = txt_pos[1]
            for i, ln in enumerate(lines):
                # choose color for the status line only
                if i == 0:
                    color = (0, 255, 0) if connected else (0, 0, 255)
                else:
                    color = (255, 255, 255)  # white for ball coords
                cv2.putText(frame, ln, (txt_pos[0], y_offset), font, font_scale, color, thickness, cv2.LINE_AA)
                # increment y_offset by height of this line + spacing
                y_offset += heights[i] + line_spacing

            cv2.imshow("Frame", frame)
            key = cv2.waitKey(1) & 0xFF
            # If user presses 'q' or ESC, break
            if key == ord('q') or key == 27:
                break

    except Exception:
        # silent on any unexpected exception
        pass

    finally:
        cleanup(cap, ser)
