# Hardware Interconnect & Wiring Schematics

This document breaks down the full flight controller hardware schematic into isolated, easy-to-read subsystem modules. 

---

## 1. Power Distribution & Voltage Telemetry Subsystem

This module handles battery power stepping and samples raw LiPo pack voltage for telemetry processing[cite: 1].

```mermaid
flowchart LR
    LIPO["3S/4S LiPo Battery Pack"] ==> PDB["Power Distribution Board"]
    PDB ==>|"11.1V - 16.8V Main Power Rail"| BEC["5V / 3A Buck Converter"]
    BEC -->|"Clean 5V Output Rail"| ESP_VIN["ESP32 VIN Pin"]
    
    PDB -->|"Raw Voltage Output"| R1["10k Ohm Resistor"]
    R1 --- R2["2.2k Ohm Resistor"]
    R2 --- GND["Common System Ground"]
    R1 ---|"Scaled Signal: 0 to 3.3V"| ESP_ADC["ESP32 GPIO 34 (ADC1_CH6)"]
```

### Module Specifications
* **Voltage Divider Ratio:** $10\text{k}\Omega / (10\text{k}\Omega + 2.2\text{k}\Omega) = 0.1803$[cite: 1]
* **Max Analog Input:** $16.8\text{V} \times 0.1803 = 3.03\text{V}$ (Safely within ESP32 3.3V ADC limit)[cite: 1]
* **Power Step-Down:** Buck converter steps raw battery power down to 5V DC at up to 3A[cite: 1].

---

## 2. Inertial Measurement Unit Interface (I2C Bus)

This module handles 6-DOF motion data acquisition from the gyro and accelerometer[cite: 1].

```mermaid
flowchart LR
    subgraph I2C_BUS["I2C Sensor Interface"]
        ESP32["ESP32 Flight Controller"]
        MPU["MPU-6050 IMU Breakout"]
        
        ESP32 -->|"3.3V VCC Rail"| MPU
        ESP32 <-->|"GPIO 21 (SDA)"| MPU
        ESP32 <-->|"GPIO 22 (SCL)"| MPU
        
        PU1["4.7k Pull-up to 3.3V"] --- ESP32
        PU2["4.7k Pull-up to 3.3V"] --- ESP32
    end
```

### Module Specifications
* **Bus Speed:** 400kHz Fast-mode I2C[cite: 1]
* **Signal Lines:** GPIO 21 (SDA), GPIO 22 (SCL)[cite: 1]
* **Hardware Requirements:** External $4.7\text{k}\Omega$ pull-up resistors on SDA/SCL lines to guarantee stable pulse transitions[cite: 1].

---

## 3. Remote Control Receiver Link (SBUS Protocol)

This module interfaces the RC receiver with the hardware UART port[cite: 1].

```mermaid
flowchart LR
    subgraph SBUS_IF["Radio Receiver Interface"]
        BEC["5V BEC Power Rail"] -->|"5V Power Input"| RX["2.4GHz SBUS Receiver"]
        GND["System Ground Bus"] --- RX
        RX -->|"100kbps Inverted Serial Stream"| ESP_RX["ESP32 GPIO 16 (UART2 RX)"]
    end
```

### Module Specifications
* **Baud Rate:** 100,000 bps (8 data bits, Even parity, 2 stop bits - 8E2)[cite: 1]
* **Input Pin:** GPIO 16 (Hardware UART2 RX)[cite: 1]
* **Operating Voltage:** 5V powered via Buck Converter[cite: 1].

---

## 4. Propulsion & ESC Actuator Block

This module routes raw high-current battery voltage to the ESCs and sends precision timing control signals from the ESP32[cite: 1].

```mermaid
flowchart TD
    subgraph PROP_SYS["Propulsion Subsystem"]
        PDB["Power Distribution Board"] ==>|"Unregulated Power Rail"| ESC1["ESC 1 - Front Left"]
        PDB ==>|"Unregulated Power Rail"| ESC2["ESC 2 - Front Right"]
        PDB ==>|"Unregulated Power Rail"| ESC3["ESC 3 - Back Left"]
        PDB ==>|"Unregulated Power Rail"| ESC4["ESC 4 - Back Right"]

        ESP["ESP32 Microcontroller"] -->|"GPIO 13 Pulse Signal"| ESC1
        ESP -->|"GPIO 12 Pulse Signal"| ESC2
        ESP -->|"GPIO 14 Pulse Signal"| ESC3
        ESP -->|"GPIO 27 Pulse Signal"| ESC4

        ESC1 ==>|"3-Phase Power Output"| M1["Motor 1 - FL (CW)"]
        ESC2 ==>|"3-Phase Power Output"| M2["Motor 2 - FR (CCW)"]
        ESC3 ==>|"3-Phase Power Output"| M3["Motor 3 - BL (CCW)"]
        ESC4 ==>|"3-Phase Power Output"| M4["Motor 4 - BR (CW)"]
    end
```

### Module Specifications
* **Signal Protocol:** Hardware MCPWM (50Hz - 400Hz update rate)[cite: 1]
* **Pulse Range:** $1000\mu\text{s}$ (Idle) to $1850\mu\text{s}$ (Max Thrust Limit)[cite: 1]
* **Pin Allocation:**
  * ESC 1 (Front Left): `GPIO 13`[cite: 1]
  * ESC 2 (Front Right): `GPIO 12`[cite: 1]
  * ESC 3 (Back Left): `GPIO 14`[cite: 1]
  * ESC 4 (Back Right): `GPIO 27`[cite: 1]

---

## 5. Visual Telemetry & LED Indicator Matrix

This module handles physical visual feedback for armed state, battery level, and fault warnings[cite: 1].

```mermaid
flowchart LR
    subgraph LED_MATRIX["Visual State Output Interface"]
        ESP["ESP32 Microcontroller"]
        
        ESP -->|"GPIO 26"| R1["100 Ohm"] --> G_LED["Green LED (System Status)"] --> GND["GND"]
        ESP -->|"GPIO 25"| R2["100 Ohm"] --> Y_LED["Yellow LED (Arm / Mode)"] --> GND
        ESP -->|"GPIO 33"| R3["100 Ohm"] --> R_LED["Red LED (Fault / Alarm)"] --> GND
    end
```

### Module Specifications
* **Current Limiting:** $100\Omega$ resistors per LED branch[cite: 1]
* **Pin Allocation:**
  * System Status (Green): `GPIO 26`[cite: 1]
  * Arm / Mode State (Yellow): `GPIO 25`[cite: 1]
  * Alarm / Fault (Red): `GPIO 33`[cite: 1]

---

## 6. Complete System Pinout Reference Table

| ESP32 Pin | Function | Connected Component | Signal Type | Electrical Parameter |
| :--- | :--- | :--- | :--- | :--- |
| `VIN` | Board Power | 5V Buck Converter Output | Power Input | 5.0V DC[cite: 1] |
| `GND` | Common Ground | Central Power Bus | Ground | 0V Common Reference[cite: 1] |
| `3V3` | Sensor Power | MPU-6050 & Pull-ups | Power Output | 3.3V DC (Max 250mA)[cite: 1] |
| `GPIO 16` | UART2 RX | SBUS Receiver Signal Output | Serial Input | 100kbps Inverted[cite: 1] |
| `GPIO 21` | I2C SDA | MPU-6050 Data Pin | Bidirectional Data | 3.3V Logic with 4.7kΩ Pull-up[cite: 1] |
| `GPIO 22` | I2C SCL | MPU-6050 Clock Pin | Bus Clock Output | 400kHz with 4.7kΩ Pull-up[cite: 1] |
| `GPIO 34` | ADC1 CH6 | Voltage Divider Node | Analog Input | Scaled 0 to 3.03V max[cite: 1] |
| `GPIO 13` | MCPWM CH0 | ESC 1 (Front Left) | PWM Signal | $1000\mu\text{s} - 1850\mu\text{s}$ pulses[cite: 1] |
| `GPIO 12` | MCPWM CH1 | ESC 2 (Front Right) | PWM Signal | $1000\mu\text{s} - 1850\mu\text{s}$ pulses[cite: 1] |
| `GPIO 14` | MCPWM CH2 | ESC 3 (Back Left) | PWM Signal | $1000\mu\text{s} - 1850\mu\text{s}$ pulses[cite: 1] |
| `GPIO 27` | MCPWM CH3 | ESC 4 (Back Right) | PWM Signal | $1000\mu\text{s} - 1850\mu\text{s}$ pulses[cite: 1] |
| `GPIO 26` | Digital Out | Green LED Circuit | Active High Output | Drives via 100Ω resistor[cite: 1] |
| `GPIO 25` | Digital Out | Yellow LED Circuit | Active High Output | Drives via 100Ω resistor[cite: 1] |
| `GPIO 33` | Digital Out | Red LED Circuit | Active High Output | Drives via 100Ω resistor[cite: 1] |
