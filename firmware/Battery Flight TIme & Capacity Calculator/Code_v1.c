#include <Arduino.h>

// --- BATTERY SPECIFICATIONS ---
const float BATTERY_CAPACITY_MAH = 2200.0f; // Nominal Capacity in mAh
const float USABLE_CAPACITY_FACTOR = 0.80f; // 80% Safety Rule to protect LiPo cells
const float MAX_USABLE_MAH = BATTERY_CAPACITY_MAH * USABLE_CAPACITY_FACTOR; // 1760 mAh

// --- QUADCOPTER CURRENT MODEL PARAMETERS ---
// Empirical estimation based on quad motor throttle PWM (1000us - 2000us)
const float IDLE_CURRENT_AMPS = 1.2f;    // Current draw with motors disarmed / idle
const float HOVER_CURRENT_AMPS = 11.5f;  // Current draw at 1460us hover throttle
const float MAX_CURRENT_AMPS = 45.0f;    // Current draw at 1000% full throttle

// System Tracking Memory
float totalConsumedMilliAmpSeconds = 0.0f;
uint32_t lastTimestampMicros = 0;

// Simulates reading motor throttle or ACS712 current sensor
float estimateCurrentDrawAmps(int throttlePwm) {
  if (throttlePwm <= 1000) return IDLE_CURRENT_AMPS;

  // Quadratic current draw model scaled by throttle output
  float throttleNormalized = (float)(throttlePwm - 1000) / 1000.0f; // 0.0 to 1.0
  float currentAmps = IDLE_CURRENT_AMPS + (MAX_CURRENT_AMPS - IDLE_CURRENT_AMPS) * (throttleNormalized * throttleNormalized);

  return currentAmps;
}

void setup() {
  Serial.begin(115200);
  lastTimestampMicros = micros();

  Serial.println("\n[SYSTEM]: Capacity & Flight Time Estimator Online.");
  Serial.print("[CONFIG]: Battery Rating: "); Serial.print(BATTERY_CAPACITY_MAH); Serial.println(" mAh");
  Serial.print("[CONFIG]: Safe Usable Limit (80%): "); Serial.print(MAX_USABLE_MAH); Serial.println(" mAh");
}

void loop() {
  uint32_t currentMicros = micros();
  float dtSeconds = (float)(currentMicros - lastTimestampMicros) / 1000000.0f;
  lastTimestampMicros = currentMicros;

  // Simulated flight throttle profile for testing estimation algorithms
  int simulatedThrottle = 1460; // Standard hover throttle setpoint

  // Calculate instantaneous current
  float currentAmps = estimateCurrentDrawAmps(simulatedThrottle);

  // Numerical integration of current over delta-time (A * s * 1000 = mA * s)
  totalConsumedMilliAmpSeconds += (currentAmps * 1000.0f) * dtSeconds;

  // Convert to milliamp-hours (mAh)
  float consumedMah = totalConsumedMilliAmpSeconds / 3600.0f;
  float remainingMah = MAX_USABLE_MAH - consumedMah;
  if (remainingMah < 0) remainingMah = 0;

  float remainingPercentage = (remainingMah / MAX_USABLE_MAH) * 100.0f;

  // Estimate remaining flight time in seconds based on current discharge rate
  float estimatedRemainingTimeMinutes = 0.0f;
  if (currentAmps > 0.1f) {
    estimatedRemainingTimeMinutes = (remainingMah / (currentAmps * 1000.0f)) * 60.0f;
  }

  // Output Telemetry Stream at 2Hz
  static int printCounter = 0;
  if (++printCounter >= 20) {
    Serial.print("Current_A:"); Serial.print(currentAmps, 2); Serial.print(",");
    Serial.print("Consumed_mAh:"); Serial.print(consumedMah, 1); Serial.print(",");
    Serial.print("Remaining_Pct:"); Serial.print(remainingPercentage, 1); Serial.print(",");
    Serial.print("Est_Time_Min:"); Serial.println(estimatedRemainingTimeMinutes, 2);
    printCounter = 0;
  }

  delay(50); // 20Hz Tracking Loop
}
