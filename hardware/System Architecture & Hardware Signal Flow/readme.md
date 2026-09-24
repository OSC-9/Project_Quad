# System Architecture & Hardware Signal Flow

This document details the complete system architecture, sensor fusion pipeline, SBUS decoder, cascade PID control loop, battery telemetry system, and LED state machine for the custom ESP32 Flight Controller.

---

## 1. Master System Hardware Topology

```mermaid
flowchart TD
    subgraph POWER["Power Distribution and Regulation"]
        LIPO["3S or 4S LiPo Battery Pack"] --> PDB["Power Distribution Board"]
        PDB -->|"V_Pack Rail"| V_DIV["Resistor Divider: 10k / 2.2k"]
        PDB -->|"Unregulated Rail"| ESC_RAIL["4x ESC Power Inputs"]
        PDB -->|"5V Regulator Output"| BEC["5V / 3A BEC Buck Converter"]
        BEC -->|"5V Rail"| ESP_5V["ESP32 VIN Pin"]
    end

    subgraph MCU["ESP32 Microcontroller Core"]
        ESP_ADC["ADC1_CH6 - GPIO 34"]
        ESP_UART["UART2 RX - GPIO 16"]
        ESP_I2C["I2C Master - GPIO 21 and 22"]
        ESP_PWM["MCPWM Timers - GPIO 12, 13, 14, 27"]
        ESP_GPIO["GPIO Outputs - GPIO 25, 26, 33"]
    end

    subgraph SENSORS["Sensors and Receivers"]
        V_DIV -->|"V_ADC Signal 0 to 3.3V"| ESP_ADC
        SBUS_RX["SBUS Remote Receiver"] -->|"100kbps Inverted Serial"| ESP_UART
        MPU["MPU-6050 IMU Sensor"] -->|"400kHz I2C Bus"| ESP_I2C
    end

    subgraph ACTUATORS["Propulsion and Output Controls"]
        ESP_PWM -->|"MCPWM 50Hz-400Hz Pulses"| ESC1["ESC 1 - Front Left"]
        ESP_PWM -->|"MCPWM 50Hz-400Hz Pulses"| ESC2["ESC 2 - Front Right"]
        ESP_PWM -->|"MCPWM 50Hz-400Hz Pulses"| ESC3["ESC 3 - Back Left"]
        ESP_PWM -->|"MCPWM 50Hz-400Hz Pulses"| ESC4["ESC 4 - Back Right"]
        
        ESC1 --> M1["Motor 1 - FL"]
        ESC2 --> M2["Motor 2 - FR"]
        ESC3 --> M3["Motor 3 - BL"]
        ESC4 --> M4["Motor 4 - BR"]
        
        ESP_GPIO --> LED_G["Green LED - System Status"]
        ESP_GPIO --> LED_Y["Yellow LED - Arm or Mode"]
        ESP_GPIO --> LED_R["Red LED - Fault or Alarm"]
    end
```

---

## 2. Sensor Fusion & Hardware Axis-Swap Pipeline

```mermaid
flowchart LR
    subgraph HW_READ["1. Raw Hardware Read"]
        MPU["MPU-6050 Registers"] -->|"I2C Read"| RAW_ACC["Raw Accel: Ax, Ay, Az"]
        MPU -->|"I2C Read"| RAW_GYRO["Raw Gyro: Gx, Gy, Gz"]
    end

    subgraph SCALING["2. Unit Scaling"]
        RAW_ACC -->|"/ 16384.0 LSB/g"| ACC_G["Accel in Gs"]
        RAW_GYRO -->|"/ 65.5 LSB/deg/s"| GYRO_DPS["Gyro in Deg/Sec"]
    end

    subgraph AXIS_SWAP["3. Frame Re-orientation"]
        ACC_G -->|"Map to Airframe"| SWAP_A["Pitch_a = -Ay<br/>Roll_a = Ax<br/>Yaw_a = Az"]
        GYRO_DPS -->|"Map to Airframe"| SWAP_G["pitch_rate = -Gy<br/>roll_rate = Gx<br/>yaw_rate = Gz"]
    end

    subgraph TRIG_EST["4. Accel Angle Trigonometry"]
        SWAP_A -->|"atan2 formula"| ACC_ANGLES["theta_acc = atan2(Ax, Az)<br/>phi_acc = atan2(Ay, Az)"]
    end

    subgraph COMP_FILTER["5. 250Hz Complementary Fusion Engine"]
        ACC_ANGLES -->|"Alpha = 0.96 Weight"| FUSION["Angle = 0.96 * Angle_Gyro + 0.04 * Accel_Angle"]
        SWAP_G -->|"Gyro Rate Component"| FUSION
    end

    FUSION -->|"Output States"| OUT["Pitch Angle in deg<br/>Roll Angle in deg<br/>Pitch Rate in deg/s<br/>Roll Rate in deg/s"]
```

---

## 3. SBUS Protocol & Remote Stick Calibration Engine

```mermaid
flowchart TD
    START["Hardware UART2 Serial Stream: 100kbps 8E2"] --> BYTE_IN["Read Byte from Ring Buffer"]
    
    BYTE_IN --> SYNC_CHECK{"Is byte == 0x0F and Index == 0?"}
    SYNC_CHECK -->|"No"| DISCARD["Advance Buffer Pointer"]
    SYNC_CHECK -->|"Yes"| STORE["Store in 25-Byte Frame Buffer"]
    
    STORE --> BUF_FULL{"Buffer Length == 25 Bytes?"}
    BUF_FULL -->|"No"| BYTE_IN
    BUF_FULL -->|"Yes"| FOOTER_CHECK{"Byte 24 == 0x00?"}
    
    FOOTER_CHECK -->|"Invalid"| FLUSH["Reset Buffer Index to 0"]
    FOOTER_CHECK -->|"Valid"| UNPACK["Unpack 16 Channels via 11-Bit Bitmask"]
    
    UNPACK --> FLAGS["Parse Byte 23 Flags<br/>Frame Lost or Failsafe Bit"]
    
    UNPACK --> MAP_PWM["Linear Mapping Transformation<br/>u = 1000 + (Raw - 173) / 1639 * 1000"]
    
    MAP_PWM --> DEADBAND["Apply Center Deadband: abs(u - 1500) <= 8us"]
    
    DEADBAND --> FAILSAFE_CHECK{"Failsafe Flag Active?"}
    FAILSAFE_CHECK -->|"Yes"| SET_EMERGENCY["Set Output Throttle = 1000us<br/>Trigger System Disarm"]
    FAILSAFE_CHECK -->|"No"| OUTPUT_VALS["Publish Channel Commands<br/>CH1: Roll, CH2: Pitch, CH3: Throttle, CH4: Yaw"]
```

---

## 4. Dual-Axis Cascade PID Controller & Motor Mixer

```mermaid
flowchart TD
    subgraph INPUTS["Target Commands and Sensor Feedback"]
        STICK_ANG["Stick Input Target Angles in deg"]
        EST_ANG["Complementary Estimated Angles in deg"]
        GYRO_RATES["Filtered Gyro Rates in deg/s"]
    end

    subgraph OUTER_LOOP["Outer Loop: Angle P-Control"]
        STICK_ANG --> ANG_ERR_CALC["Angle Error = Setpoint - Estimated"]
        EST_ANG --> ANG_ERR_CALC
        ANG_ERR_CALC --> P_OUTER["Target Rate = Kp_angle * Angle Error"]
    end

    subgraph INNER_LOOP["Inner Loop: Rate PID Control at 250Hz"]
        P_OUTER --> RATE_ERR_CALC["Rate Error = Target Rate - Gyro Rate"]
        GYRO_RATES --> RATE_ERR_CALC
        
        RATE_ERR_CALC --> P_INNER["P_term = Kp_rate * Rate Error"]
        RATE_ERR_CALC --> I_INNER["I_term = I_term + Ki_rate * Rate Error * dt"]
        RATE_ERR_CALC --> D_INNER["D_term = Kd_rate * (Rate Error - Prev_Error) / dt"]
        
        P_INNER --> AXIS_SUM["u_axis = P_term + I_term + D_term"]
        I_INNER --> AXIS_SUM
        D_INNER --> AXIS_SUM
    end

    subgraph MIXER["X-Frame Motor Pulse Mixer Matrix"]
        AXIS_SUM -->|"u_pitch / u_roll"| MATRIX["M_FL = Auto_Throttle + u_pitch + u_roll<br/>M_FR = Auto_Throttle + u_pitch - u_roll<br/>M_BL = Auto_Throttle - u_pitch + u_roll<br/>M_BR = Auto_Throttle - u_pitch - u_roll"]
    end

    subgraph SAFETY["Output Saturation and Clamp"]
        MATRIX --> CLAMP["Clamp M_k between 1000us and 1850us"]
        CLAMP --> MCPWM["Write MCPWM Hardware Registers"]
    end
```

---

## 5. Battery Telemetry & Capacity Integrator

```mermaid
flowchart TD
    subgraph VOLTAGE_PATH["Voltage Telemetry Processing"]
        HW_ADC["GPIO 34 Analog Input"] --> OVERSAMPLE["32x Hardware Over-sampling"]
        OVERSAMPLE --> DIV_CALC["V_ADC = Sample / 4095.0 * 3.3V"]
        DIV_CALC --> PACK_CALC["V_Pack = V_ADC * (12.2 / 2.2) * K_cal"]
        PACK_CALC --> MOV_AVG["20-Sample Moving Average Filter"]
    end

    subgraph CAPACITY_PATH["Current Estimation and Capacity Integrator"]
        PWM_IN["Motor Throttle PWM Command u"] --> CURR_MODEL["Estimated Current I(u) = I_idle + (I_max - I_idle) * ((u - 1000)/1000)^2"]
        CURR_MODEL --> DISCRETE_INT["Discrete Riemann Integrator<br/>C_consumed = sum(I_k * 1000 * dt / 3600)"]
        DISCRETE_INT --> REM_TIME["Remaining Time T_rem = (C_usable - C_consumed) / (I_current * 1000) * 60"]
    end

    subgraph THRESHOLDS["Safety Evaluation Engine"]
        MOV_AVG --> COMP_THRES{"Compare V_Pack per Cell"}
        COMP_THRES -->|"Cell >= 3.50V"| BATT_OK["State = Normal Power"]
        COMP_THRES -->|"3.30V to 3.50V"| BATT_WARN["State = Low Voltage Warning"]
        COMP_THRES -->|"Cell < 3.30V"| BATT_CRIT["State = Critical Cutoff Fault"]
    end
```

---

## 6. LED Indication & System State Machine

```mermaid
stateDiagram-v2
    [*] --> DISARMED_IDLE

    state DISARMED_IDLE {
        [*] --> Idle_Blink
        Idle_Blink: Green LED Blink at 1Hz
        Idle_Blink: Yellow LED OFF
        Idle_Blink: Red LED OFF
    }

    state ARMED_HEALTHY {
        [*] --> Armed_Blink
        Armed_Blink: Green LED Solid ON
        Armed_Blink: Yellow LED Flash at 5Hz
        Armed_Blink: Red LED OFF
    }

    state WARNING_LOW_BATTERY {
        [*] --> Warn_Blink
        Warn_Blink: Green LED Solid ON
        Warn_Blink: Yellow LED Solid ON
        Warn_Blink: Red LED Flash at 2Hz
    }

    state CRITICAL_FAULT {
        [*] --> Fault_Strobe
        Fault_Strobe: Green LED OFF
        Fault_Strobe: Yellow LED OFF
        Fault_Strobe: Red LED High Strobe at 10Hz
    }

    DISARMED_IDLE --> ARMED_HEALTHY : Arm Switch ON and Failsafe OK
    ARMED_HEALTHY --> DISARMED_IDLE : Arm Switch OFF
    ARMED_HEALTHY --> WARNING_LOW_BATTERY : V_Cell < 3.50V
    WARNING_LOW_BATTERY --> CRITICAL_FAULT : V_Cell < 3.30V or Frame Lost
    DISARMED_IDLE --> CRITICAL_FAULT : Sensor Init Failed or Invalid SBUS
```
