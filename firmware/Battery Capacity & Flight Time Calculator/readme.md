## Non-Blocking 3-LED Status Indicator

> [!WARNING]
> This indicator module uses non-blocking hardware timing registers (`millis()`) instead of delay functions (`delay()`). This guarantees zero latency jitter for the primary 250Hz flight control loop.

---

### 1. System Timing Equation

$$t_{\text{toggle}} = \frac{1000}{2 \cdot f_{\text{blink}}} \quad [\text{ms}]$$

State update condition:

  $$\Delta t_{\text{elapsed}} = t_{\text{current}} - t_{\text{last}} \ge t_{\text{toggle}}$$
  
---


### 2. Flight State Output Matrix

| System State | Green LED (`GPIO 27`) | Yellow LED (`GPIO 26`) | Red LED (`GPIO 32`) | Visual Pattern |
| :--- | :--- | :--- | :--- | :--- |
| `DISARMED_IDLE` | Flash ($1\text{Hz}$) | OFF | OFF | System Standby |
| `ARMED_HEALTHY` | Solid ON | Flash ($5\text{Hz}$) | OFF | Quad Armed / Flight Ready |
| `WARNING_LOW_BATTERY` | Solid ON | Solid ON | Flash ($2\text{Hz}$) | Low Battery Alert ($<3.50\text{V}$) |
| `CRITICAL_FAULT` | OFF | OFF | Strobe ($10\text{Hz}$) | Critical Error / Emergency Land |

---

### 3. Hardware Pinout Reference

| LED Signal | Color | ESP32 GPIO | Resistor | Driver Current |
| :--- | :--- | :--- | :--- | :--- |
| **Power / Heartbeat** | Green | `GPIO 27` | $330\,\Omega$ | $5.0\,\text{mA}$ |
| **Arm / Mode** | Yellow | `GPIO 26` | $330\,\Omega$ | $5.0\,\text{mA}$ |
| **Fault / Alarm** | Red | `GPIO 32` | $330\,\Omega$ | $5.0\,\text{mA}$ |
