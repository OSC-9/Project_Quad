## SENSOR FUSION & ORIENTATION DIAGNOSTIC


1. SYSTEM OVERVIEW
------------------
This diagnostic module reads 6-axis IMU telemetry from the MPU6050 over fast I2C 
and applies sensor fusion via a Complementary Filter at 250Hz[cite: 18]. It handles 
hardware axis re-mapping for rotated sensor mountings and prevents bus contention[cite: 18].

2. MATHEMATICAL SENSOR FUSION
-----------------------------
The Complementary Filter merges high-frequency angular rate integration with low-frequency 
accelerometer orientation vectors[cite: 18]:

$$\theta_{\text{comp}}(k) = \alpha \cdot \left(\theta_{\text{comp}}(k-1) + \dot{\theta}_{\text{gyro}} \cdot \Delta t\right) + (1 - \alpha) \cdot \theta_{\text{acc}}$$

$$\phi_{\text{comp}}(k) = \alpha \cdot \left(\phi_{\text{comp}}(k-1) + \dot{\phi}_{\text{gyro}} \cdot \Delta t\right) + (1 - \alpha) \cdot \phi_{\text{acc}}$$

Where:
  * Filter Coefficient: \alpha = 0.96[cite: 18]
  * Loop Execution Time Step: \Delta t = 0.004\text{s} \quad (250\text{Hz})[cite: 18]
  * \theta_{\text{acc}} = \text{atan2}(a_y, a_z) \cdot \frac{180}{\pi} \quad \text{(Pitch Angle)}[cite: 18]
  * \phi_{\text{acc}} = \text{atan2}(-a_x, a_z) \cdot \frac{180}{\pi} \quad \text{(Roll Angle)}

3. HARDWARE INTERRUPT MITIGATION
--------------------------------
To prevent register locks caused by unhandled hardware interrupts on MPU6050:
  * Interrupt Enable Register `0x38` is written to `0x00`[cite: 18].
  * Microcontroller GPIO 25 is assigned to high-impedance `INPUT` mode[cite: 18].
  * Fast-Mode I2C bus speed is initialized at 400kHz on SDA GPIO 21 and SCL GPIO 22.
