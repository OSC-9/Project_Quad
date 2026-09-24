# SBUS Protocol Receiver & Control Stick Engine

> [!NOTE]
> This module decodes Futaba/FrSky 100kbps inverted UART SBUS frames, unpacks 16 discrete 11-bit radio channels, maps bitfield integers to standard $1000\mu\text{s} - 2000\mu\text{s}$ RC PWM control commands, and evaluates hardware failsafe flags.

---

## 1. System Architecture
Remote Transmitter (TX) | (2.4GHz RF) | SBUS RX Receiver
│
(Inverted 100kbps 8E2)
│
ESP32 GPIO 16 (UART2 RX)
│
[ 11-Bit Frame Unpacker ]
│
[ Stick Deadband Filter ]
│
PWM Signals (1000µs - 2000µs)


---

## 2. Mathematical Equations & Signal Scaling

### Linear Raw-to-PWM Conversion Equation
Transforms the unpacked 11-bit raw integer $R_{\text{channel}} \in [173, 1812]$ to microsecond pulse width duration $u(t) \in [1000\mu\text{s}, 2000\mu\text{s}]$:

$$u(t) = \text{PWM}_{\text{min}} + \left( \frac{R_{\text{channel}} - R_{\text{min}}}{R_{\text{max}} - R_{\text{min}}} \right) \cdot (\text{PWM}_{\text{max}} - \text{PWM}_{\text{min}})$$

Where:
* $R_{\text{min}} = 173$ ($1000\mu\text{s}$ raw lower bound)
* $R_{\text{max}} = 1812$ ($2000\mu\text{s}$ raw upper bound)
* $\text{PWM}_{\text{min}} = 1000\mu\text{s}$
* $\text{PWM}_{\text{max}} = 2000\mu\text{s}$

### Stick Center Deadband Filter
Suppresses physical gimbal potentiometer noise around neutral mid-stick ($1500\mu\text{s}$):

$$u_{\text{filtered}}(t) = \begin{cases} 1500 & \text{if } \vert{}u(t) - 1500\vert{} \le \text{Deadband} \\ u(t) & \text{otherwise} \end{cases}$$

Where:
* $\text{Deadband} = 8\mu\text{s}$

---

## 3. SBUS Protocol Frame Bit Structure

A complete SBUS packet consists of **25 sequential bytes** transmitted every $9\text{ms} - 14\text{ms}$:

| Byte Index | Field Description | Logical Value / Bit Range |
| :--- | :--- | :--- |
| `Byte 0` | Header Byte | `0x0F` (Sync Byte) |
| `Bytes 1–22` | Packed Channels 1 to 16 | $16 \times 11\text{ bits} = 176\text{ bits}$ total payload |
| `Byte 23` | System Status Flags | Bit 0: CH17, Bit 1: CH18, Bit 2: Frame Lost, Bit 3: Failsafe |
| `Byte 24` | Footer Byte | `0x00` (End Byte) |

---

## 4. Default RC Channel Assignment Matrix

| Channel | Function | Microsecond Range | Operational Context |
| :--- | :--- | :--- | :--- |
| **CH1** | Roll Axis | $1000\mu\text{s} - 2000\mu\text{s}$ | Aileron Control ($1500\mu\text{s}$ Center) |
| **CH2** | Pitch Axis | $1000\mu\text{s} - 2000\mu\text{s}$ | Elevator Control ($1500\mu\text{s}$ Center) |
| **CH3** | Throttle Axis | $1000\mu\text{s} - 2000\mu\text{s}$ | Motor Power Output ($1000\mu\text{s}$ Zero Thrust) |
| **CH4** | Yaw Axis | $1000\mu\text{s} - 2000\mu\text{s}$ | Rudder Control ($1500\mu\text{s}$ Center) |
| **CH5** | Arm Switch | $1000\mu\text{s} / 2000\mu\text{s}$ | Auxiliary 1 (Disarmed / Armed state) |
| **CH6** | Flight Mode | $1000\mu\text{s} - 2000\mu\text{s}$ | Auxiliary 2 (Acro / Angle / Horizon) |

---

## 5. Hardware Configuration

| Parameter | Value | Hardware Notes |
| :--- | :--- | :--- |
| **Target Hardware** | `ESP32 UART2` | Uses ESP32 internal inverted UART logic |
| **RX Pin** | `GPIO 16` | Connected directly to SBUS Receiver Signal Pin |
| **Baud Rate** | `100000 bps` | Standard inverted non-RS232 serial rate |
| **Framing** | `8E2` | 8 Data Bits, Even Parity, 2 Stop Bits |
