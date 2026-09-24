#include <Arduino.h>
#include <ESP32Servo.h>

const int PIN_MOT_FR = 19; // Motor 1: Front Right[cite: 23]
const int PIN_MOT_BR = 14; // Motor 2: Rear Right [cite: 23]
const int PIN_MOT_BL = 13; // Motor 3: Rear Left  [cite: 23]
const int PIN_MOT_FL = 18; // Motor 4: Front Left [cite: 23]

Servo mFR, mBR, mBL, mFL;

const int TEST_SPEED_PWM = 1150; // Safe low throttle speed for visual check[cite: 23]

void shutdownMotors() {
  mFR.writeMicroseconds(1000);
  mBR.writeMicroseconds(1000);
  mBL.writeMicroseconds(1000);
  mFL.writeMicroseconds(1000);
}

void setup() {
  Serial.begin(115200);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  mFR.attach(PIN_MOT_FR, 1000, 2000);
  mBR.attach(PIN_MOT_BR, 1000, 2000);
  mBL.attach(PIN_MOT_BL, 1000, 2000);
  mFL.attach(PIN_MOT_FL, 1000, 2000);

  shutdownMotors();

  Serial.println("\n[SYSTEM]: Motor Isolation Diagnostic Sequence Starting...");
  Serial.println("[SAFETY]: Ensuring all motors remain off for 5 seconds...");
  delay(5000); // 5-second window to power up LiPo safely[cite: 23]
}

void loop() {
  // Test Motor 1: Front Right
  Serial.println("\n>>> [ACTIVE]: Motor 1 (Front Right | Pin 19)");
  mFR.writeMicroseconds(TEST_SPEED_PWM);
  delay(3000); // Spin for 3 seconds[cite: 23]
  shutdownMotors();
  delay(1000);

  // Test Motor 2: Rear Right
  Serial.println(">>> [ACTIVE]: Motor 2 (Rear Right | Pin 14)");
  mBR.writeMicroseconds(TEST_SPEED_PWM);
  delay(3000); // Spin for 3 seconds[cite: 23]
  shutdownMotors();
  delay(1000);

  // Test Motor 3: Rear Left
  Serial.println(">>> [ACTIVE]: Motor 3 (Rear Left | Pin 13) [CHECK INTEGRITY]");
  mBL.writeMicroseconds(TEST_SPEED_PWM);
  delay(3000); // Spin for 3 seconds[cite: 23]
  shutdownMotors();
  delay(1000);

  // Test Motor 4: Front Left
  Serial.println(">>> [ACTIVE]: Motor 4 (Front Left | Pin 18)");
  mFL.writeMicroseconds(TEST_SPEED_PWM);
  delay(3000); // Spin for 3 seconds[cite: 23]
  shutdownMotors();
  delay(1000);

  Serial.println("[CYCLE COMPLETE]: Waiting 4 seconds before repeating...");
  delay(4000);
}
