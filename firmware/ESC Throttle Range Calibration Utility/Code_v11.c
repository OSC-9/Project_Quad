#include <Arduino.h>
#include <ESP32Servo.h>

// --- HARDWARE PIN DEFINITIONS ---
const int PIN_MOT_FR = 19; // Front Right Motor[cite: 20]
const int PIN_MOT_BR = 14; // Back Right Motor [cite: 20]
const int PIN_MOT_BL = 13; // Back Left Motor  [cite: 20]
const int PIN_MOT_FL = 18; // Front Left Motor [cite: 20]

Servo motorFR, motorBR, motorBL, motorFL;

// Helper: Broadcasts PWM microsecond pulse to all 4 ESCs simultaneously
void updateAllMotors(int microseconds) {
  motorFR.writeMicroseconds(microseconds);
  motorBR.writeMicroseconds(microseconds);
  motorBL.writeMicroseconds(microseconds);
  motorFL.writeMicroseconds(microseconds);
}

void setup() {
  Serial.begin(115200);

  // Allocate ESP32 hardware PWM timers (0-3) for jitter-free signal generation
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // Set standard 50Hz refresh rate for analog/SimonK ESCs
  motorFR.setPeriodHertz(50);
  motorBR.setPeriodHertz(50);
  motorBL.setPeriodHertz(50);
  motorFL.setPeriodHertz(50);

  // Attach servo library instances to hardware pins with 1000us - 2000us limits
  motorFR.attach(PIN_MOT_FR, 1000, 2000);
  motorBR.attach(PIN_MOT_BR, 1000, 2000);
  motorBL.attach(PIN_MOT_BL, 1000, 2000);
  motorFL.attach(PIN_MOT_FL, 1000, 2000);

  // Default to safe zero signal on boot
  updateAllMotors(1000);

  Serial.println("\n==================================================");
  Serial.println("     ESC THROTTLE RANGE CALIBRATION UTILITY      ");
  Serial.println("==================================================");
  Serial.println("1. DISCONNECT LiPo battery!");
  Serial.println("2. Type 'MAX' and press Enter to send 2000us signal.");
  Serial.println("3. CONNECT LiPo battery. Wait for initial 'BEEP-BEEP'.");
  Serial.println("4. Type 'MIN' and press Enter immediately after beeps.");
  Serial.println("==================================================\n");
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toUpperCase();

    if (command == "MAX") {
      updateAllMotors(2000);
      Serial.println("\n[STATUS]: Signal set to 2000us (MAX THROTTLE).");
      Serial.println("[ACTION]: Connect LiPo NOW! Type 'MIN' right after the double beep.");
    } 
    else if (command == "MIN") {
      updateAllMotors(1000);
      Serial.println("\n[STATUS]: Signal set to 1000us (MIN THROTTLE / IDLE).");
      Serial.println("[SUCCESS]: Calibration sequence complete. Listen for confirmation chime.");
      Serial.println("[TEST]: Type 'TEST' to spin all motors at safe low idle (1100us).");
    } 
    else if (command == "TEST") {
      updateAllMotors(1100);
      Serial.println("\n[STATUS]: Spinning all motors at 1100us idle.");
      Serial.println("[ACTION]: Type 'MIN' to halt all motors.");
    } 
    else {
      updateAllMotors(1000);
      Serial.println("\n[WARNING]: Unknown command! Emergency shutdown forced (1000us).");
    }
  }
}
