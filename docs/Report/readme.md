# Project Quad Report 

> An end-to-end, bare-metal quadcopter flight control system engineered from first principles on the ESP32 WROOM-32 dual-core microcontroller, based on the specifications outlined in the project file Quad. Designed to meet the rigorous physical and computational demands of high-performance, heavy-lift, and tactical-grade humanitarian payloads, this platform explicitly abandons consumer flight stacks. It implements real-time hardware-level serial protocol decoding, deterministic RTOS scheduling, dual-axis cascade PID control loops, and multi-tier failsafe redundancies natively in C/C++.

---

## 1. System Architecture & Tactical Objectives

Project Quad establishes absolute deterministic authority over signal processing and actuator dynamics. The architecture strictly isolates high-frequency control execution from asynchronous telemetry and communication tasks across the ESP32's dual cores, eliminating race conditions and ensuring guaranteed execution deadlines. 

### 1.1 Core Specifications
* **Processing Subsystem:** ESP32 Xtensa Dual-Core 32-bit LX6 Microprocessor operating at 240 MHz.
* **Control Determinism:** 250 Hz fixed control loop (strict 4.00 ms execution deadline) pinned to Core 1.
* **Command Link Latency:** Sub-15 ms stick-to-motor execution via DMA-backed UART SBUS decoding.
* **Sensor Acquisition:** 400 kHz Fast-mode I2C pipeline utilizing a custom complementary filter ($\alpha=0.96$).
* **Actuator Topology:** Hardware Motor Control PWM (MCPWM) driving four discrete 30A BLHeli_S Electronic Speed Controllers (ESCs).
* **Power Diagnostics:** Continuous high-resolution 12-bit analog voltage sampling with 32x hardware oversampling for brownout prevention.

---

## 2. Hardware Interface & Electrical Topology

To guarantee electromagnetic interference (EMI) immunity in high-current operational envelopes, the hardware footprint is rigidly segmented. Logic-level signals are physically and electrically isolated from the main propulsion power plane.

### 2.1 Power Distribution & Voltage Telemetry Matrix
Manages raw high-current loads while providing precision low-voltage analog telemetry to the MCU.

| Subsystem Component | Electrical Path / Interface | Technical Specification | Operational Theory & Limits |
| :--- | :--- | :--- | :--- |
| **Main Power Matrix** | 3S/4S LiPo $\rightarrow$ PDB | 11.1V - 16.8V Rail | Direct distribution to 30A ESCs. Power delivery paths are isolated from the MCU logic planes to prevent inductive spiking. |
| **Logic Regulation** | PDB $\rightarrow$ 5V Buck $\rightarrow$ `VIN` | 5V / 3A Output | Steps down the primary battery rail to provide a stable, low-noise power supply to the ESP32 LDO and RF receiver. |
| **Voltage Divider (High)** | PDB Main Rail $\rightarrow$ 10 kΩ | $R_1 = 10\text{ k}\Omega$ (1% Tol) | First stage of analog scaling. Shields the sensitive 3.3V ESP32 ADC logic gates from direct pack voltage. |
| **Voltage Divider (Low)** | Divider Node $\rightarrow$ 2.2 kΩ $\rightarrow$ GND | $R_2 = 2.2\text{ k}\Omega$ (1% Tol) | Completes the voltage divider network to set the definitive analog scale factor. |
| **ADC Interrogation** | Divider Node $\rightarrow$ GPIO 34 | ADC1_CH6 (12-bit) | Configured for 11dB attenuation (0-3.3V range). Samples at 100 Hz with hardware oversampling to filter ESC switching noise. |

### 2.2 Inertial Measurement Unit (IMU) Array
Extracts raw spatial orientation data via the MPU-6050 6-Degree-of-Freedom MEMS sensor.

| Subsystem Component | Electrical Path / Interface | Technical Specification | Operational Theory & Limits |
| :--- | :--- | :--- | :--- |
| **Sensor Power** | ESP32 3.3V $\rightarrow$ `VCC` | 3.3V Dedicated Rail | Driven directly by the ESP32 onboard LDO. Bypasses the 5V buck converter to avoid ripple interference. |
| **Data Line (SDA)** | ESP32 GPIO 21 $\rightarrow$ `SDA` | I2C Bidirectional | Open-drain architecture requiring an external 4.7 kΩ pull-up resistor for sharp logic-high transitions. |
| **Clock Line (SCL)** | ESP32 GPIO 22 $\rightarrow$ `SCL` | I2C Clock Source | Driven by the ESP32 at 400 kHz (Fast-mode) to minimize bus blocking time during the 250 Hz control loop. |
| **Gyroscope Range** | Internal Register `0x1B` | ±500°/s | Configured for a scale factor of 65.5 LSB/°/s. Optimized for rapid attitude correction without saturation. |
| **Accelerometer Range**| Internal Register `0x1C` | ±8g | Configured for a scale factor of 4096 LSB/g. Provides gravity vector stability against severe motor vibration. |

### 2.3 Command & Control (C2) Link (SBUS Protocol)
Decodes inverted, high-speed serial data natively without CPU-blocking interrupts.

| Subsystem Component | Electrical Path / Interface | Technical Specification | Operational Theory & Limits |
| :--- | :--- | :--- | :--- |
| **Receiver Logic Power**| 5V Buck Rail $\rightarrow$ RX `VCC` | 5V Input | Guarantees continuous 2.4GHz RF module lock during extreme throttle voltage sag. |
| **Serial Signal Link** | RX Out $\rightarrow$ ESP32 GPIO 16 | UART2 RX Port | Routes directly to the ESP32 UART2 hardware FIFO buffer. |
| **Protocol Mechanics** | Serial UART Config | 100 kbps, 8E2 | Operates at 100,000 baud, 8 data bits, Even parity, 2 stop bits. |
| **Logic Inversion** | Software/Hardware Invert | Inverted TTL | SBUS natively uses inverted logic. Handled via UART hardware inversion registers in the ESP-IDF/Arduino core. |

### 2.4 Propulsion Actuation Matrix (MCPWM)
Bypasses standard software timers in favor of the Motor Control PWM (MCPWM) peripheral for jitter-free, microsecond-accurate pulse generation.

| Subsystem Component | Electrical Path / Interface | Technical Specification | Operational Theory & Limits |
| :--- | :--- | :--- | :--- |
| **Motor 1 (Front Left)** | ESP32 GPIO 13 $\rightarrow$ ESC 1 | MCPWM0, Channel 0 | Commands Clockwise (CW) rotation. |
| **Motor 2 (Front Right)**| ESP32 GPIO 12 $\rightarrow$ ESC 2 | MCPWM0, Channel 1 | Commands Counter-Clockwise (CCW) rotation. |
| **Motor 3 (Back Left)** | ESP32 GPIO 14 $\rightarrow$ ESC 3 | MCPWM0, Channel 2 | Commands Counter-Clockwise (CCW) rotation. |
| **Motor 4 (Back Right)** | ESP32 GPIO 27 $\rightarrow$ ESC 4 | MCPWM0, Channel 3 | Commands Clockwise (CW) rotation. |
| **Pulse Envelope** | Hardware Timer Bounds | 1000 µs - 1850 µs | 1000 µs asserts absolute motor stop; 1850 µs acts as an artificial ceiling to prevent ESC desynchronization under high load. |

### 2.5 Visual Diagnostics & Failsafe Feedback
Outputs autonomous, line-of-sight state feedback independent of a digital ground control station.

| Subsystem Component | Electrical Path / Interface | Technical Specification | Operational Theory & Limits |
| :--- | :--- | :--- | :--- |
| **System Active (Green)**| GPIO 26 $\rightarrow$ 100 Ω $\rightarrow$ GND | Active High LED | 1 Hz Blink = Boot/Gyro Calibration. Solid = System nominal, loop active. |
| **Arm Status (Yellow)** | GPIO 25 $\rightarrow$ 100 Ω $\rightarrow$ GND | Active High LED | Solid = Armed. 5 Hz Flash = Active flight control override / Thrust engaged. |
| **Critical Fault (Red)** | GPIO 33 $\rightarrow$ 100 Ω $\rightarrow$ GND | Active High LED | 2 Hz Flash = Low Battery Warning. 10 Hz Strobe = Critical Failsafe Disarm Invoked. |

---

## 3. Low-Level Protocol Integration

### 3.1 I2C Register Map & DMA Transfer
The primary loop reads 14 consecutive bytes starting from the MPU-6050 `ACCEL_XOUT_H` (`0x3B`) register in a single hardware transaction to minimize clock overhead.
* `0x3B` to `0x40`: Accelerometer High/Low Bytes (X, Y, Z)
* `0x41` to `0x42`: Temperature High/Low Bytes (Discarded in processing)
* `0x43` to `0x48`: Gyroscope High/Low Bytes (X, Y, Z)

### 3.2 SBUS Frame Anatomy
The C2 link transmits data in 25-byte packets. The UART peripheral buffers these, and a dedicated Core 0 task unpacks the 11-bit channels.
* **Byte 0:** Start Byte (`0x0F`)
* **Bytes 1-22:** 16 multiplexed channels of 11-bit resolution payload.
* **Byte 23:** Flags (Bit 7: Frame Lost, Bit 8: Failsafe Activated).
* **Byte 24:** End Byte (`0x00`).

---

## 4. Firmware Topology & RTOS Core Pinning

The system utilizes FreeRTOS to enforce a strict preemptive multitasking environment.

### Core 1: Real-Time Flight Engine (Priority 24)
Dedicated entirely to physical flight dynamics. No asynchronous data waits are permitted.
1. **Acquire:** Pulls the 14-byte I2C FIFO payload.
2. **Filter:** Executes the $\alpha=0.96$ Complementary Filter.
3. **Compute:** Processes Pitch, Roll, and Yaw vectors through the Dual-Axis Cascade PID logic.
4. **Mix & Actuate:** Calculates the X-frame geometry matrix and commits values to the MCPWM timer hardware.
5. **Yield:** Calls `vTaskDelayUntil()` to block the task exactly until the next 4.00 ms interval, ensuring mathematically perfect integral calculation in the PID.

### Core 0: Asynchronous Telemetry & C2 (Priority 5 - 15)
Manages ground communications and safety logic.
* **SBUS Task (Priority 15):** Continuously reads the UART2 DMA ring buffer, reconstructs the 11-bit channels, maps them to standard 1000-2000 µs float domains, and loads them into a thread-safe FreeRTOS Queue.
* **Telemetry Task (Priority 10):** Samples GPIO 34 at 100 Hz. Calculates the moving average for battery voltage, assesses brownout limits, and updates the GPIO LED matrix.

---

## 5. Mathematical Engine & Sensor Fusion

### 5.1 Power Estimation & Scaling Matrix
Raw ADC values are reconstructed into standard voltage units for the failsafe logic state machine.

$$K_{\text{div}} = \frac{R_2}{R_1 + R_2} = \frac{2.2}{10 + 2.2} \approx 0.180327$$
$$V_{\text{Pack}} = \left( \frac{\text{ADC}_{\text{Raw}} \times 3.3}{4095} \right) \times \frac{1}{K_{\text{div}}}$$

Estimated electrical current draw $I(u)$ is mathematically derived based on the commanded motor pulse $u \in [1000, 1850]$ to anticipate voltage sag:
$$I(u) = I_{\text{idle}} + (I_{\text{max}} - I_{\text{idle}}) \cdot \left(\frac{u - 1000}{850}\right)^2$$

### 5.2 Sensor Fusion: The Complementary Filter
A computationally lightweight complementary filter resolves frame orientation by merging high-frequency gyroscope data (which drifts over time) with low-frequency accelerometer data (which is highly susceptible to high-RPM motor noise).

**Raw Accelerometer Euler Angle Derivation:**
$$\theta_{\text{acc}} = \arctan\left(\frac{a_y}{a_z}\right), \quad \phi_{\text{acc}} = \arctan\left(\frac{-a_x}{\sqrt{a_y^2 + a_z^2}}\right)$$

**Filter Integration Matrix:**
$$\begin{bmatrix} \theta_{\text{fused}}(t) \\ \phi_{\text{fused}}(t) \end{bmatrix} = \alpha \cdot \left( \begin{bmatrix} \theta_{\text{fused}}(t-1) \\ \phi_{\text{fused}}(t-1) \end{bmatrix} + \begin{bmatrix} \omega_x \\ \omega_y \end{bmatrix} \cdot \Delta t \right) + (1 - \alpha) \cdot \begin{bmatrix} \theta_{\text{acc}} \\ \phi_{\text{acc}} \end{bmatrix}$$

*Note: $\alpha = 0.96$ heavily biases the gyroscope for responsive transient maneuvers while leveraging the accelerometer gravity vector to zero out steady-state integration drift.*

### 5.3 Cascade PID Control Architecture
Flight stability relies on a nested dual-loop Proportional-Integral-Derivative (PID) controller. The outer loop dictates the vehicle's position (attitude), and the inner loop calculates the rotational torque required to reach that position.

**Outer Loop (Attitude / Position Control):**
$$e_{\text{angle}}(t) = \theta_{\text{target}}(t) - \theta_{\text{fused}}(t)$$
$$\omega_{\text{target}}(t) = K_{p,\text{angle}} \cdot e_{\text{angle}}(t)$$

**Inner Loop (Rate / Gyro Control):**
$$e_{\text{rate}}(t) = \omega_{\text{target}}(t) - \omega_{\text{gyro}}(t)$$
$$U_{\text{PID}}(t) = K_{p,\text{rate}} \cdot e_{\text{rate}}(t) + K_{i,\text{rate}} \int_0^t e_{\text{rate}}(\tau) d\tau + K_{d,\text{rate}} \cdot \frac{d}{dt}\left( e_{\text{rate}}(t) \right)$$

---

## 6. Digital Signal Processing (DSP) & Harmonic Filtering

To scale this bare-metal platform for heavy-lift industrial rotors, raw sensor data must be scrubbed of mechanical resonance before entering the PID loop. 

* **Biquad Low-Pass Filter (PT1):** Applied directly to the $D$-term of the inner rate loop. The derivative calculation strictly amplifies high-frequency motor noise. Applying a discrete-time First-Order Low-Pass Filter (PT1) at a 80 Hz cutoff frequency smooths the torque output, preventing motor oscillation (hot motors) without introducing unacceptable phase delay.
* **Software Notch Filtering:** Heavy robotics exhibit specific resonant frequencies based on frame geometry and propeller pitch. A software-defined notch filter is positioned mathematically just after gyro acquisition to aggressively attenuate the exact frequency band of the propulsion system's mechanical hum.

---

## 7. Actuator Kinematics & Mixer Geometry

The independent outputs of the PID controller ($U_{\text{Pitch}}$, $U_{\text{Roll}}$, $U_{\text{Yaw}}$) are dynamically mixed with the pilot's base throttle input to calculate specific PWM duty cycles for the X-frame motor layout.

$$\begin{bmatrix} \text{ESC}_1 \\ \text{ESC}_2 \\ \text{ESC}_3 \\ \text{ESC}_4 \end{bmatrix} = \begin{bmatrix} 1 & +1 & +1 & -1 \\ 1 & +1 & -1 & +1 \\ 1 & -1 & +1 & +1 \\ 1 & -1 & -1 & -1 \end{bmatrix} \begin{bmatrix} U_{\text{Throttle}} \\ U_{\text{Pitch}} \\ U_{\text{Roll}} \\ U_{\text{Yaw}} \end{bmatrix}$$

**Hardware Clamping Matrix:**
To prevent CPU integer overflow and guarantee the MCPWM hardware does not transmit an invalid signal (which would trigger a mid-flight ESC reboot), the final outputs are rigidly clamped.
$$\text{ESC}_n = \max(1000, \min(\text{ESC}_n, 1850)) \quad \text{for } n \in \{1, 2, 3, 4\}$$

---

## 8. Tactical Humanitarian & Industrial Payload Integration

Project Quad is structured as a foundational flight module specifically engineered for heavy robotics and UNODA/UNOPS humanitarian deployments (e.g., autonomous medical supply drops, Unexploded Ordnance (UXO) scanning). 

* **EMI & Environmental Hardening:** The logic boards must be encased in a grounded carbon/copper Faraday cage to prevent RF interference from 30A+ power lines. Conformal coating (acrylic or silicone) is applied to the ESP32 and MPU-6050 to prevent logic-level short circuits in high-humidity or dust-heavy operational theaters.
* **Payload Agnosticism:** Because the state-estimation (Core 1) is entirely decoupled from external comms (Core 0), secondary microcontrollers (like a Jetson Orin Nano) can be bridged via UART to send autonomous $\theta_{\text{target}}$ navigation commands without ever jeopardizing the primary low-level flight loop stability.
* **Vibration Isolation:** The MPU-6050 is suspended on optimized silicone vibration dampeners (calibrated for the drone's specific mass) to physically reject frame resonance before it ever hits the digital DSP filters.

---

## 9. Failsafe State Machine & Tactical Redundancy

A catastrophic failure in telemetry or signal link will immediately trigger hardware interlocks.

| Operational State | Invocation Condition | Firmware Execution & Motor Logic | Visual Feedback Output |
| :--- | :--- | :--- | :--- |
| **STATE_BOOT** | Power On / MCU Hard Reset | Executes 1000-cycle IMU bias calibration. PID integrators initialized to 0. Motors locked mechanically to 1000 µs. | Green LED: 1 Hz Blink |
| **STATE_DISARMED** | C2 Arm Switch Logic Low | Flight loops run in simulation only. Actuator output bypassed. Throttle override actively held at 1000 µs. | Green LED: Solid |
| **STATE_ARMED** | C2 Arm Switch Logic High + Valid RF Frame | Cascade PID loops unlatched. Base idle thrust elevated to 1050 µs to prevent stalling during freefall maneuvers. | Yellow LED: 5 Hz Strobe |
| **WARNING_VOLT** | Battery Cell $< 3.50\text{V}$ | Normal flight vectors maintained. Integral windup limits constricted dynamically to preserve remaining chemical energy. | Red LED: 2 Hz Flash |
| **CRITICAL_DROP** | SBUS Frame Lost $> 500\text{ms}$ OR Cell $< 3.30\text{V}$ | Hardware timer interlock engaged. I-terms dumped, P-terms zeroed. All actuators forcefully cut to 1000 µs to prevent uncontrolled trajectory. | Red LED: 10 Hz Strobe |
