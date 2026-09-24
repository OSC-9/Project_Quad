#include <Arduino.h>

// --- HARDWARE PIN DEFINITIONS ---
const int PIN_LED_GREEN  = 27; // System Power / Heartbeat Indicator
const int PIN_LED_YELLOW = 26; // Arming State / Flight Mode Indicator
const int PIN_LED_RED    = 32; // Fault / Low Battery Alert Indicator

// System Flight States
enum SystemState {
  DISARMED_IDLE,
  ARMED_HEALTHY,
  WARNING_LOW_BATTERY,
  CRITICAL_FAULT
};

SystemState currentState = DISARMED_IDLE;

// Non-blocking timer tracking registers
uint32_t lastGreenToggleMs  = 0;
uint32_t lastYellowToggleMs = 0;
uint32_t lastRedToggleMs    = 0;

bool stateGreen  = LOW;
bool stateYellow = LOW;
bool stateRed    = LOW;

void updateStatusLEDs(SystemState state) {
  uint32_t currentMs = millis();

  switch (state) {
    case DISARMED_IDLE:
      // Green: Slow Pulse (1Hz) | Yellow: OFF | Red: OFF
      if (currentMs - lastGreenToggleMs >= 500) {
        stateGreen = !stateGreen;
        digitalWrite(PIN_LED_GREEN, stateGreen);
        lastGreenToggleMs = currentMs;
      }
      digitalWrite(PIN_LED_YELLOW, LOW);
      digitalWrite(PIN_LED_RED, LOW);
      break;

    case ARMED_HEALTHY:
      // Green: Solid ON | Yellow: Rapid Flash (5Hz) | Red: OFF
      digitalWrite(PIN_LED_GREEN, HIGH);
      if (currentMs - lastYellowToggleMs >= 100) {
        stateYellow = !stateYellow;
        digitalWrite(PIN_LED_YELLOW, stateYellow);
        lastYellowToggleMs = currentMs;
      }
      digitalWrite(PIN_LED_RED, LOW);
      break;

    case WARNING_LOW_BATTERY:
      // Green: Solid ON | Yellow: Solid ON | Red: Medium Flash (2Hz)
      digitalWrite(PIN_LED_GREEN, HIGH);
      digitalWrite(PIN_LED_YELLOW, HIGH);
      if (currentMs - lastRedToggleMs >= 250) {
        stateRed = !stateRed;
        digitalWrite(PIN_LED_RED, stateRed);
        lastRedToggleMs = currentMs;
      }
      break;

    case CRITICAL_FAULT:
      // Green: OFF | Yellow: OFF | Red: Hyper Flash (10Hz Strobe)
      digitalWrite(PIN_LED_GREEN, LOW);
      digitalWrite(PIN_LED_YELLOW, LOW);
      if (currentMs - lastRedToggleMs >= 50) {
        stateRed = !stateRed;
        digitalWrite(PIN_LED_RED, stateRed);
        lastRedToggleMs = currentMs;
      }
      break;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);

  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_YELLOW, LOW);
  digitalWrite(PIN_LED_RED, LOW);

  Serial.println("\n[SYSTEM]: 3-LED Status Indicator Module Ready.");
}

void loop() {
  // Demo state cycle loop for hardware testing
  uint32_t sec = millis() / 1000;
  if (sec < 5) {
    currentState = DISARMED_IDLE;
  } else if (sec < 10) {
    currentState = ARMED_HEALTHY;
  } else if (sec < 15) {
    currentState = WARNING_LOW_BATTERY;
  } else {
    currentState = CRITICAL_FAULT;
  }

  updateStatusLEDs(currentState);
}
