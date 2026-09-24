#include <Arduino.h>
#include <Wire.h>
#include <ESP32Servo.h>
#include "sbus.h" // Bolder Flight Systems SBUS library[cite: 21, 22]

// --- HARDWARE MAPPING ---
const int PIN_MOT_FL = 18; // Front Left [cite: 21, 22]
const int PIN_MOT_FR = 19; // Front Right[cite: 21, 22]
const int PIN_MOT_BL = 13; // Back Left  [cite: 21, 22]
const int PIN_MOT_BR = 14; // Back Right [cite: 21, 22]
const int PIN_RC_RX  = 26; // Receiver Input RX2[cite: 18, 21, 22]

// Hardware Instances
bfs::SbusRx sbusReceiver(&Serial2, PIN_RC_RX, 27, true); // Hardware Serial 2, inverted logic[cite: 21, 22]
bfs::SbusData sbusFrame;[cite: 21, 22]

Servo mFL, mFR, mBL, mBR;

// Control Constraints & Spool Targets
const float TARGET_HOVER_THROTTLE = 1460.0f;[cite: 21, 22]
const float MAX_ALLOWED_PWM       = 1850.0f; // Caged safety RPM limit[cite: 21]
const float DT                    = 0.004f;  // Strict 250Hz cycle time (4ms)[cite: 18, 19, 21, 22]

// Control Loop Gains (Pitch & Roll)
float PRatePitch = 3.50f, IRatePitch = 0.08f, DRatePitch = 0.0020f;[cite: 21]
float PAnglePitch = 3.00f, IAnglePitch = 0.10f, DAnglePitch = 0.0000f;[cite: 21]

float PRateRoll  = 3.50f, IRateRoll  = 0.08f, DRateRoll  = 0.0020f;[cite: 21]
float PAngleRoll  = 3.00f, IAngleRoll  = 0.10f, DAngleRoll  = 0.0000f;[cite: 21]

// Receiver Mapping Arrays
volatile int rcChannels[6] = {1500, 1500, 1000, 1500, 1000, 1000}; // Roll, Pitch, Throttle, Yaw, Arm, Switch[cite: 18, 21, 22]

// IMU State Registers
float gyroPitchRate, gyroRollRate;
float accPitch, accRoll;
float compPitch = 0.0f, compRoll = 0.0f;

// PID Tracking Memory Registers
float prevItermAnglePitch = 0, prevErrAnglePitch = 0;[cite: 18, 21, 22]
float prevItermRatePitch  = 0, prevErrRatePitch  = 0;[cite: 18, 21, 22]

float prevItermAngleRoll  = 0, prevErrAngleRoll  = 0;[cite: 21]
float prevItermRateRoll   = 0, prevErrRateRoll   = 0;[cite: 21]

float currentAutoThrottle = 1000.0f;[cite: 21, 22]
uint32_t executionTimer;[cite: 18, 21, 22]

void readRCReceiver() {
  if (sbusReceiver.Read()) {[cite: 21, 22]
    sbusFrame = sbusReceiver.data();[cite: 21, 22]
    // Map raw SBUS values (282-1722) to standard PWM pulse widths (1000us-2000us)[cite: 21, 22]
    rcChannels[0] = map(sbusFrame.ch[0], 282, 1722, 1000, 2000); // Roll[cite: 21, 22]
    rcChannels[1] = map(sbusFrame.ch[1], 282, 1722, 1000, 2000); // Pitch[cite: 21, 22]
    rcChannels[2] = map(sbusFrame.ch[2], 282, 1722, 1000, 2000); // Throttle[cite: 21, 22]
    rcChannels[3] = map(sbusFrame.ch[3], 282, 1722, 1000, 2000); // Yaw[cite: 21, 22]
    rcChannels[4] = map(sbusFrame.ch[4], 282, 1722, 1000, 2000); // Arm Switch (AUX1)[cite: 21, 22]
  }
}

void readIMUData() {
  Wire.beginTransmission(0x68);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom((uint16_t)0x68, (uint8_t)14, true);

  if (Wire.available() == 14) {
    int16_t rAx = Wire.read() << 8 | Wire.read();
    int16_t rAy = Wire.read() << 8 | Wire.read();
    int16_t rAz = Wire.read() << 8 | Wire.read();
    Wire.read() << 8 | Wire.read(); // Skip Temp
    int16_t rGx = Wire.read() << 8 | Wire.read();
    int16_t rGy = Wire.read() << 8 | Wire.read();
    int16_t rGz = Wire.read() << 8 | Wire.read();

    float ax = (float)rAx / 4096.0f;[cite: 18, 19, 21, 22]
    float ay = (float)rAy / 4096.0f;[cite: 18, 19, 21, 22]
    float az = (float)rAz / 4096.0f;[cite: 18, 19, 21, 22]

    gyroPitchRate = ((float)rGx / 65.5f); // Hardware X mapped to Pitch Rate[cite: 18, 19, 21, 22]
    gyroRollRate  = ((float)rGy / 65.5f); // Hardware Y mapped to Roll Rate[cite: 21]

    accPitch = (atan2(ay, az) * 57.29578f);[cite: 18, 21, 22]
    accRoll  = (atan2(-ax, az) * 57.29578f);[cite: 21]

    // Complementary Filter Fusion[cite: 18, 21, 22]
    compPitch = 0.96f * (compPitch + gyroPitchRate * DT) + 0.04f * accPitch;[cite: 18, 21, 22]
    compRoll  = 0.96f * (compRoll  + gyroRollRate  * DT) + 0.04f * accRoll; [cite: 21]
  }
}

void setup() {
  Serial.begin(115200);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  mFL.attach(PIN_MOT_FL, 1000, 2000); mFL.setPeriodHertz(50);[cite: 21, 22]
  mFR.attach(PIN_MOT_FR, 1000, 2000); mFR.setPeriodHertz(50);[cite: 21, 22]
  mBL.attach(PIN_MOT_BL, 1000, 2000); mBL.setPeriodHertz(50);[cite: 21, 22]
  mBR.attach(PIN_MOT_BR, 1000, 2000); mBR.setPeriodHertz(50);[cite: 21, 22]

  // Safe Idle Signal Boot Command[cite: 21, 22]
  mFL.writeMicroseconds(1000); mFR.writeMicroseconds(1000);[cite: 21, 22]
  mBL.writeMicroseconds(1000); mBR.writeMicroseconds(1000);[cite: 21, 22]

  sbusReceiver.Begin();[cite: 21, 22]

  // MPU6050 Low-level setup[cite: 18, 19, 21, 22]
  Wire.setClock(400000); Wire.begin();[cite: 19, 21, 22]
  Wire.beginTransmission(0x68); Wire.write(0x6B); Wire.write(0x00); Wire.endTransmission();[cite: 18, 19, 21, 22]
  Wire.beginTransmission(0x68); Wire.write(0x1B); Wire.write(0x08); Wire.endTransmission();[cite: 18, 19, 21, 22]
  Wire.beginTransmission(0x68); Wire.write(0x1A); Wire.write(0x03); Wire.endTransmission();[cite: 18, 19, 21, 22]
  Wire.beginTransmission(0x68); Wire.write(0x38); Wire.write(0x00); Wire.endTransmission();[cite: 18, 19, 21, 22]

  executionTimer = micros();[cite: 18, 21, 22]
}

void loop() {
  readRCReceiver();[cite: 21, 22]
  readIMUData();[cite: 21, 22]

  int armStateSwitch = rcChannels[4]; // Channel 5 Arm Check[cite: 21, 22]

  if (armStateSwitch < 1500) {[cite: 18, 21, 22]
    // ==========================================
    // DISARMED: HARDWARE SAFETY KILL ACTIVE[cite: 21, 22]
    // ==========================================
    currentAutoThrottle = 1000.0f;[cite: 21, 22]
    mFL.writeMicroseconds(1000); mFR.writeMicroseconds(1000);[cite: 21, 22]
    mBL.writeMicroseconds(1000); mBR.writeMicroseconds(1000);[cite: 21, 22]

    // Flush Integrator and Derivative Memory to prevent spool-up windup[cite: 18, 21, 22]
    prevItermAnglePitch = 0; prevErrAnglePitch = 0;[cite: 18, 21, 22]
    prevItermRatePitch  = 0; prevErrRatePitch  = 0;[cite: 18, 21, 22]
    prevItermAngleRoll  = 0; prevErrAngleRoll  = 0;[cite: 21]
    prevItermRateRoll   = 0; prevErrRateRoll   = 0;[cite: 21]
  } 
  else {
    
    // Smooth Auto-Ramp Spooler (0.1% per loop up to hover throttle target)[cite: 21, 22]
    if (currentAutoThrottle < TARGET_HOVER_THROTTLE) {
      currentAutoThrottle += 1.0f;[cite: 21, 22]
    } else {
      currentAutoThrottle = TARGET_HOVER_THROTTLE;[cite: 21, 22]
    }

    // Setpoint Calculation: Stick center (1500us) mapped to +/- 50 degree angle target[cite: 18, 21, 22]
    float setpointAnglePitch = 0.10f * (rcChannels[1] - 1500);[cite: 18, 21, 22]
    float setpointAngleRoll  = 0.10f * (rcChannels[0] - 1500);[cite: 21]

    // Outer Angle Loop[cite: 21]
    float errAnglePitch = setpointAnglePitch - compPitch;[cite: 18, 21, 22]
    float pTermAngleP   = PAnglePitch * errAnglePitch;[cite: 18, 21, 22]
    float iTermAngleP   = constrain(prevItermAnglePitch + (IAnglePitch * (errAnglePitch + prevErrAnglePitch) * (DT / 2.0f)), -400.0f, 400.0f);[cite: 18, 21, 22]
    float dTermAngleP   = DAnglePitch * ((errAnglePitch - prevErrAnglePitch) / DT);[cite: 18, 21, 22]
    float targetRatePitch = constrain(pTermAngleP + iTermAngleP + dTermAngleP, -400.0f, 400.0f);[cite: 18, 21, 22]
    prevErrAnglePitch   = errAnglePitch; prevItermAnglePitch = iTermAngleP;[cite: 18, 21, 22]

    // Inner Rate Loop[cite: 21]
    float errRatePitch  = targetRatePitch - gyroPitchRate;[cite: 18, 21, 22]
    float pTermRateP    = PRatePitch * errRatePitch;[cite: 18, 21, 22]
    float iTermRateP    = constrain(prevItermRatePitch + (IRatePitch * (errRatePitch + prevErrRatePitch) * (DT / 2.0f)), -400.0f, 400.0f);[cite: 18, 21, 22]
    float dTermRateP    = DRatePitch * ((errRatePitch - prevErrRatePitch) / DT);[cite: 18, 21, 22]
    float pitchCorrection = constrain(pTermRateP + iTermRateP + dTermRateP, -800.0f, 800.0f);[cite: 21, 22]
    prevErrRatePitch    = errRatePitch; prevItermRatePitch = iTermRateP;[cite: 18, 21, 22]

    // Outer Angle Loop[cite: 21]
    float errAngleRoll  = setpointAngleRoll - compRoll;[cite: 21]
    float pTermAngleR   = PAngleRoll * errAngleRoll;[cite: 21]
    float iTermAngleR   = constrain(prevItermAngleRoll + (IAngleRoll * (errAngleRoll + prevErrAngleRoll) * (DT / 2.0f)), -400.0f, 400.0f);[cite: 21]
    float dTermAngleR   = DAngleRoll * ((errAngleRoll - prevErrAngleRoll) / DT);[cite: 21]
    float targetRateRoll = constrain(pTermAngleR + iTermAngleR + dTermAngleR, -400.0f, 400.0f);[cite: 21]
    prevErrAngleRoll    = errAngleRoll; prevItermAngleRoll = iTermAngleR;[cite: 21]

    // Inner Rate Loop[cite: 21]
    float errRateRoll   = targetRateRoll - gyroRollRate;[cite: 21]
    float pTermRateR    = PRateRoll * errRateRoll;[cite: 21]
    float iTermRateR    = constrain(prevItermRateRoll + (IRateRoll * (errRateRoll + prevErrRateRoll) * (DT / 2.0f)), -400.0f, 400.0f);[cite: 21]
    float dTermRateR    = DRateRoll * ((errRateRoll - prevErrRateRoll) / DT);[cite: 21]
    float rollCorrection  = constrain(pTermRateR + iTermRateR + dTermRateR, -800.0f, 800.0f);[cite: 21]
    prevErrRateRoll     = errRateRoll; prevItermRateRoll = iTermRateR;[cite: 21]

    float mixFL = currentAutoThrottle + pitchCorrection + rollCorrection;[cite: 21]
    float mixFR = currentAutoThrottle + pitchCorrection - rollCorrection;[cite: 21]
    float mixBL = currentAutoThrottle - pitchCorrection + rollCorrection;[cite: 21]
    float mixBR = currentAutoThrottle - pitchCorrection - rollCorrection;[cite: 21]

    // Write Clamped Outputs to Servos/ESCs[cite: 21]
    mFL.writeMicroseconds(constrain(mixFL, 1000.0f, MAX_ALLOWED_PWM));[cite: 21]
    mFR.writeMicroseconds(constrain(mixFR, 1000.0f, MAX_ALLOWED_PWM));[cite: 21]
    mBL.writeMicroseconds(constrain(mixBL, 1000.0f, MAX_ALLOWED_PWM));[cite: 21]
    mBR.writeMicroseconds(constrain(mixBR, 1000.0f, MAX_ALLOWED_PWM));[cite: 21]
  }

  while (micros() - executionTimer < (DT * 1000000.0f)) {
    readRCReceiver(); // Process incoming SBUS serial bytes during idle time[cite: 18, 21, 22]
  }
  executionTimer = micros();[cite: 18, 21, 22]
}
