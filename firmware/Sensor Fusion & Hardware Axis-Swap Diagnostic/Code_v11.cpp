#include <Arduino.h>
#include <Wire.h>

const int MPU_ADDR = 0x68;
const float DT = 0.004f; // 250Hz loop time step (4ms)[cite: 18, 19, 21, 22]

float gyroXRate, gyroYRate, gyroZRate;
float accX, accY, accZ;
float accPitchAngle, accRollAngle;
float compPitchAngle = 0.0f, compRollAngle = 0.0f;

void initMPU6050() {
  Wire.begin(21, 22); // SDA=21, SCL=22[cite: 19]
  Wire.setClock(400000); // Fast I2C Mode (400kHz)[cite: 19, 21, 22]

  // Power Management 1: Wake up device
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission();

  // Gyroscope Config: +/- 500 deg/s sensitivity (65.5 LSB / deg/s)
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1B);
  Wire.write(0x08);
  Wire.endTransmission();

  // Accelerometer Config: +/- 8g sensitivity (4096 LSB / g)
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C);
  Wire.write(0x10);
  Wire.endTransmission();

  // Low-Pass Filter Config: ~42Hz Bandwidth
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1A);
  Wire.write(0x03);
  Wire.endTransmission();

  // CRITICAL HARDWARE FIX: Disable MPU INT pin driver to prevent pin contention
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x38);
  Wire.write(0x00);
  Wire.endTransmission();
}

void readAndProcessIMU() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // Starting register for Accelerometer data
  Wire.endTransmission(false);
  Wire.requestFrom((uint16_t)MPU_ADDR, (uint8_t)14, true);

  if (Wire.available() == 14) {
    int16_t rawAccX = Wire.read() << 8 | Wire.read();
    int16_t rawAccY = Wire.read() << 8 | Wire.read();
    int16_t rawAccZ = Wire.read() << 8 | Wire.read();
    Wire.read() << 8 | Wire.read(); // Skip temperature registers
    int16_t rawGyroX = Wire.read() << 8 | Wire.read();
    int16_t rawGyroY = Wire.read() << 8 | Wire.read();
    int16_t rawGyroZ = Wire.read() << 8 | Wire.read();

    // Scale Raw Data based on hardware registers
    accX = (float)rawAccX / 4096.0f;
    accY = (float)rawAccY / 4096.0f;
    accZ = (float)rawAccZ / 4096.0f;

    // HARDWARE TO SOFTWARE AXIS RE-MAPPING
    // Raw GyroX mapped directly to software Pitch Rate[cite: 18, 19]
    gyroXRate = (float)rawGyroX / 65.5f; 
    gyroYRate = (float)rawGyroY / 65.5f;
    gyroZRate = (float)rawGyroZ / 65.5f;

    // Trigonometric Euler Angle calculation using swapped acceleration axes[cite: 18, 19]
    accPitchAngle = atan2(accY, accZ) * 57.29578f; // Convert Radians to Degrees
    accRollAngle  = atan2(-accX, accZ) * 57.29578f;[cite: 21]

    // 96% Gyro Integration + 4% Accelerometer Vector Fusion (Complementary Filter)
    compPitchAngle = 0.96f * (compPitchAngle + gyroXRate * DT) + 0.04f * accPitchAngle;[cite: 18, 21, 22]
    compRollAngle  = 0.96f * (compRollAngle + gyroYRate * DT)  + 0.04f * accRollAngle;[cite: 21]
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(25, INPUT); // Set MPU_INT_PIN as high-impedance input[cite: 18]
  initMPU6050();
  Serial.println("[SYSTEM]: IMU Initialized. Pitch and Roll tracking live.");
}

void loop() {
  uint32_t loopStart = micros();

  readAndProcessIMU();

  static int printDivider = 0;
  if (++printDivider >= 10) { // Print telemetry at 25Hz
    Serial.print("RawAccPitch:");  Serial.print(accPitchAngle);   Serial.print(",");
    Serial.print("CompPitch:");     Serial.print(compPitchAngle);  Serial.print(",");
    Serial.print("CompRoll:");      Serial.println(compRollAngle);
    printDivider = 0;
  }

  // Maintain 250Hz Loop rate
  while (micros() - loopStart < (DT * 1000000.0f)) { }
}
