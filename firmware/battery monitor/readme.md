## Battery Voltage Monitor


### 1. System Architecture

V_LiPo (Pack Input) ────[ R1: 10kΩ ]────┬────[ R2: 2.2kΩ ]──── GND
│
└─── GPIO 34 (ADC1 Channel 6)

### 2. Mathematical Equations

#### Voltage Divider Equation
$$V_{\text{ADC}} = V_{\text{Pack}} \cdot \left( \frac{R_2}{R_1 + R_2} \right)$$

#### Transformed Pack Voltage
$$V_{\text{Pack}} = V_{\text{ADC}} \cdot \left( \frac{R_1 + R_2}{R_2} \right) \cdot K_{\text{cal}}$$

Where:
* $R_1 = 10000\,\Omega$ ($10\text{k}\Omega$)
* $R_2 = 2200\,\Omega$ ($2.2\text{k}\Omega$)
* Voltage Divider Ratio: $\frac{R_1 + R_2}{R_2} = 5.545$
* Maximum Input Voltage: $3.3\text{V} \times 5.545 = 18.30\text{V}$
* $K_{\text{cal}} = 1.024$ (Resistor Calibration Factor)

#### Moving Average Filter
$$\bar{V}_k = \frac{1}{N} \sum_{i=0}^{N-1} V_{k-i}$$

---

## 3. Hardware Specifications

| Parameter | Specification | Hardware Detail |
| :--- | :--- | :--- |
| **ADC Pin** | `GPIO 34` | ADC1_CH6 (Input-only, Wi-Fi safe) |
| **Attenuation** | `11 dB` | 0V - 3.3V Full Scale Range |
| **Sampling Rate** | `10 Hz` | 100ms Telemetry Loop Interval |
| **Upper Resistor ($R_1$)** | `10 kΩ` | 1/4W 1% Precision Metal Film |
| **Lower Resistor ($R_2$)** | `2.2 kΩ` | 1/4W 1% Precision Metal Film |

---

## 4. LiPo Threshold Reference

| Pack Config | Nominal | Full Charge ($4.20\text{V}$) | Warning Threshold ($3.50\text{V}$) | Critical Cutoff ($3.30\text{V}$) |
| :--- | :--- | :--- | :--- | :--- |
| **3S LiPo** | $11.10\text{V}$ | $12.60\text{V}$ | $10.50\text{V}$ | $9.90\text{V}$ |
| **4S LiPo** | $14.80\text{V}$ | $16.80\text{V}$ | $14.00\text{V}$ | $13.20\text{V}$ |
