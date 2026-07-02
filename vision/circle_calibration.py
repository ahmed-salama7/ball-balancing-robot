
import cv2

# Open default camera (index 0)
cap = cv2.VideoCapture(0)

# Frame size (make sure your camera supports 640x480)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

center = (320, 240)  # center of frame
radius = 50
color = (0, 255, 0)  # green
thickness = 2

while True:
    ret, frame = cap.read()
    if not ret:
        break

    # Draw circle
    cv2.circle(frame, center, radius, color, thickness)

    # Draw a small red dot at the center for reference
    cv2.circle(frame, center, 5, (0, 0, 255), -1)

    cv2.imshow("Camera with Circle", frame)

    # Press 'q' to exit
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()

