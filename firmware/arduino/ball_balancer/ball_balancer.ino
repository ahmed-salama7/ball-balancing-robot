#include <Servo.h>
#include "InverseKinematics.h"
#include <math.h>

// ----------------- Machine geometry (cm) -----------------
Machine machine(5.7, 8.5, 3.1, 9.66); // (d, e, f, g)

// ----------------- Servo pins & objects -----------------
const int servoPinA = 9;
const int servoPinB = 10;
const int servoPinC = 11;
Servo servoA, servoB, servoC;

// ----------------- Servo calibration -----------------
int servoNeutralA = 74;
int servoNeutralB = 97;
int servoNeutralC = 83;

int servoSignA = 1;
int servoSignB = 1;
int servoSignC = 1;

const int servoMin = 40; // safe travel lower bound
const int servoMax = 100; // safe travel upper bound

// ----------------- PID controller -----------------
// double kp = 0.0009; // proportional per pixel (Base acceptle response but high overshoot bubt can be stable with no hzat)
// double ki = 0.0;
// double kd = 0.0001;
// double kp = 0.0008; // proportional per pixel //form the above less overshoot but still oscilation a touch of it  
// double ki = 0.00000001;
// double kd = 0.0001;
double kp = 0.0004; // 
double ki = 0.0;
double kd = 0.0000;
double errorArr[2] = { 0, 0 }, errorPrev[2] = { 0, 0 }, integr[2] = { 0, 0 }, deriv[2] = { 0, 0 }, out[2] = { 0, 0 };

// baseline platform height (cm) used by IK — tune if needed
double platformHz = 10.25;

// max tilt slope (safety)
const double maxSlope = 0.6;

// loop timing
const unsigned long LOOP_PERIOD_MS = 20UL; // 50 Hz

// baseline IK thetas for flat platform
double theta0A = 0, theta0B = 0, theta0C = 0;

// serial parsing helper
String serialLine = "";

// center of camera in pixels
const double centerX = 320;
const double centerY = 240;

// ----------------- Parse "(x,y)" from Python -----------------
bool parseXYfromLine(const String &s, double &xOut, double &yOut) {
  String tmp = s;
  tmp.trim(); // remove leading/trailing whitespace
  if (tmp.length() == 0) return false;

  // Allow either "(x,y)" or "x,y" (with optional spaces)
  if (tmp.startsWith("(") && tmp.endsWith(")")) {
    tmp = tmp.substring(1, tmp.length() - 1);
    tmp.trim();
  }

  int commaIndex = tmp.indexOf(',');
  if (commaIndex < 0) return false;

  String xs = tmp.substring(0, commaIndex);
  String ys = tmp.substring(commaIndex + 1);

  xs.trim();
  ys.trim();

  // If either is empty, fail
  if (xs.length() == 0 || ys.length() == 0) return false;

  xOut = xs.toDouble();
  yOut = ys.toDouble();

  // Optionally: check for NaN (toDouble returns 0 on failure, so best-effort)
  return true;
}

// ----------------- Map IK theta -> servo write -----------------
void moveTo(double hz, double nx, double ny) {
  if (nx > maxSlope) nx = maxSlope;
  if (nx < -maxSlope) nx = -maxSlope;
  if (ny > maxSlope) ny = maxSlope;
  if (ny < -maxSlope) ny = -maxSlope;

  double thetaA = machine.theta(0, hz, nx, ny);
  double thetaB = machine.theta(1, hz, nx, ny);
  double thetaC = machine.theta(2, hz, nx, ny);

  int servoAngleA = servoNeutralA + servoSignA * (int)round(thetaA - theta0A);
  int servoAngleB = servoNeutralB + servoSignB * (int)round(thetaB - theta0B);
  int servoAngleC = servoNeutralC + servoSignC * (int)round(thetaC - theta0C);

  servoAngleA = constrain(servoAngleA, servoMin, servoMax);
  servoAngleB = constrain(servoAngleB, servoMin, servoMax);
  servoAngleC = constrain(servoAngleC, servoMin, servoMax);

  servoA.write(servoAngleA);
  servoB.write(servoAngleB);
  servoC.write(servoAngleC);

  // debug
  Serial.print("Servo: ");
  Serial.print(servoAngleA);
  Serial.print(", ");
  Serial.print(servoAngleB);
  Serial.print(", ");
  Serial.println(servoAngleC);
}

// ----------------- PID controller -----------------
void PID_compute(double ballX_px, double ballY_px) {
  errorPrev[0] = errorArr[0];
  errorPrev[1] = errorArr[1];

  // error = center - ball position
  errorArr[0] = centerX - ballX_px;
  errorArr[1] = centerY - ballY_px;

  integr[0] += (errorArr[0] + errorPrev[0]) * (LOOP_PERIOD_MS / 1000.0);
  integr[1] += (errorArr[1] + errorPrev[1]) * (LOOP_PERIOD_MS / 1000.0);

  deriv[0] = (errorArr[0] - errorPrev[0]) / (LOOP_PERIOD_MS / 1000.0);
  deriv[1] = (errorArr[1] - errorPrev[1]) / (LOOP_PERIOD_MS / 1000.0);

  out[0] = kp * errorArr[0] + ki * integr[0] + kd * deriv[0];
  out[1] = kp * errorArr[1] + ki * integr[1] + kd * deriv[1];

  out[0] = constrain(out[0], -maxSlope, maxSlope);
  out[1] = constrain(out[1], -maxSlope, maxSlope);
}

void setup() {
  Serial.begin(115200);
  delay(50);

  servoA.attach(servoPinA);
  servoB.attach(servoPinB);
  servoC.attach(servoPinC);

  servoA.write(servoNeutralA);
  servoB.write(servoNeutralB);
  servoC.write(servoNeutralC);
  delay(300);

  theta0A = machine.theta(0, platformHz, 0.0, 0.0);
  theta0B = machine.theta(1, platformHz, 0.0, 0.0);
  theta0C = machine.theta(2, platformHz, 0.0, 0.0);

  Serial.println("Ball-Balancer ready. Waiting for Python pixel data...");
}

void loop() {
  static unsigned long lastLoop = 0;
  static unsigned long lastSerialTime = 0;
  unsigned long now = millis();

  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      String line = serialLine;
      serialLine = "";
      double bx, by;
      if (parseXYfromLine(line, bx, by)) {
        lastSerialTime = millis();
        PID_compute(bx, by);
        moveTo(platformHz, -out[0], -out[1]);
      }
    } else {
      serialLine += c;
      if (serialLine.length() > 200) serialLine = serialLine.substring(serialLine.length() - 200);
    }
  }

  if (millis() - lastLoop >= LOOP_PERIOD_MS) {
    lastLoop = millis();
    if (millis() - lastSerialTime < 500) {
      moveTo(platformHz, -out[0], -out[1]);
    } else {
      moveTo(platformHz, 0.0, 0.0);
    }
  }
}
