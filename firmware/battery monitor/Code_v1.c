#include <Arduino.h>

// --- HARDWARE PINOUT & CALIBRATION ---
const int PIN_BATTERY_ADC = 34; // ADC1 Channel 6 (Input Only, Wi-Fi safe)
const float R1 = 10000.0f;     // 10k Ohm upper resistor
const float R2 = 2200.0f;      // 2.2k Ohm lower resistor
const float ADC_REF_VOLTAGE = 3.30f;
const float ADC_RESOLUTION = 4095.0f;
const float CALIBRATION_FACTOR = 1.024f; // Trimming offset for resistor tolerances

// Cell Thresholds for 3S / 4S LiPo Configurations
const int LIPO_CELL_COUNT = 3; // Change to 4 for 4S LiPo
const float CELL_WARNING_VOLTAGE = 3.50f;  // Alarm trigger per cell
const float CELL_CRITICAL_VOLTAGE = 3.30f; // Critical threshold per cell

// Moving Average Filter Setup
const int FILTER_SAMPLES = 20;
float voltageSamples[FILTER_SAMPLES];
int sampleIndex = 0;
float sampleSum = 0.0f;

float readRawADC() {
  // Multisampling to reduce ADC noise on ESP32
  uint32_t rawSum = 0;
  for (int i = 0; i < 32; i++) {
    rawSum += analogRead(PIN_BATTERY_ADC);
  }
  return (float)rawSum / 32.0f;
}

float calculatePackVoltage(float rawAdc) {
  // Step 1: Convert raw ADC integer to measured pin voltage
  float vAdc = (rawAdc / ADC_RESOLUTION) * ADC_REF_VOLTAGE * CALIBRATION_FACTOR;

  // Step 2: Scale voltage based on hardware voltage divider ratio
  float dividerRatio = (R1 + R2) / R2;
  float vPack = vAdc * dividerRatio;

  return vPack;
}

float getFilteredVoltage(float newVoltageSample) {
  sampleSum -= voltageSamples[sampleIndex];
  voltageSamples[sampleIndex] = newVoltageSample;
  sampleSum += voltageSamples[sampleIndex];

  sampleIndex = (sampleIndex + 1) % FILTER_SAMPLES;
  return sampleSum / (float)FILTER_SAMPLES;
}

void setup() {
  Serial.begin(115200);

  // Configure ADC pin mode and attenuation (11dB gives ~0V - 3.3V linear range)
  pinMode(PIN_BATTERY_ADC, INPUT);
  analogSetAttenuation(ADC_11db);

  // Initialize filter buffer with baseline reading
  float initialRead = calculatePackVoltage(readRawADC());
  for (int i = 0; i < FILTER_SAMPLES; i++) {
    voltageSamples[i] = initialRead;
    sampleSum += initialRead;
  }

  Serial.println("\n[SYSTEM]: Battery Voltage Monitor Initialized.");
}

void loop() {
  float rawAdc = readRawADC();
  float instantVoltage = calculatePackVoltage(rawAdc);
  float filteredVoltage = getFilteredVoltage(instantVoltage);
  float cellVoltage = filteredVoltage / (float)LIPO_CELL_COUNT;

  Serial.print("ADC_Raw:"); Serial.print(rawAdc); Serial.print(",");
  Serial.print("Pack_Voltage:"); Serial.print(filteredVoltage); Serial.print(",");
  Serial.print("Cell_Voltage:"); Serial.println(cellVoltage);

  if (cellVoltage <= CELL_CRITICAL_VOLTAGE) {
    Serial.println("  [ALERT]: CRITICAL BATTERY VOLTAGE - LAND IMMEDIATELY!");
  } else if (cellVoltage <= CELL_WARNING_VOLTAGE) {
    Serial.println("  [WARNING]: LOW BATTERY VOLTAGE - PREPARE TO LAND");
  }

  delay(100); // 10Hz Telemetry Rate
}
