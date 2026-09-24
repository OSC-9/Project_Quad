## Battery Flight Time & Capacity Calculator

> [!IMPORTANT]
> This module tracks cumulative power consumption in $\text{mAh}$ using discrete numerical integration over time ($\Delta t$). It enforces the **80% Depth of Discharge (DoD) Rule** to prevent flight power cutoffs and cell damage.

---

### 1. Mathematical Equations

### Discrete Capacity Integral
$$C_{\text{consumed}} = \frac{1}{3600} \sum_{k=1}^{N} \left( I_k \cdot 1000 \cdot \Delta t_k \right)$$

Where:
* $I_k$ = Instantaneous Current Draw ($\text{A}$)
* $\Delta t_k$ = Delta Loop Time ($\text{s}$)
* Scale Factor $\frac{1000}{3600}$ = Converts $\text{A}\cdot\text{s}$ to $\text{mAh}$

#### Throttle Current Draw Model
$$I(u) = I_{\text{idle}} + (I_{\text{max}} - I_{\text{idle}}) \cdot \left( \frac{u(t) - 1000}{1000} \right)^2$$

Where:
* $u(t) \in [1000\mu\text{s}, 2000\mu\text{s}]$ (Motor PWM Command Pulse Width)
* $I_{\text{idle}} = 1.2\text{A}$
* $I_{\text{max}} = 45.0\text{A}$

#### Remaining Flight Duration
$$T_{\text{rem}} = \left( \frac{C_{\text{usable}} - C_{\text{consumed}}}{I_{\text{current}} \cdot 1000} \right) \cdot 60$$

---

## 2. Power Profile Parameters

| Parameter | Value | Definition |
| :--- | :--- | :--- |
| **Nominal Capacity ($C_{\text{nominal}}$)** | `2200 mAh` | Battery Label Rating |
| **Usable Capacity ($C_{\text{usable}}$)** | `1760 mAh` | 80% Safe Discharge Limit |
| **Idle Current ($I_{\text{idle}}$)** | `1.2 A` | System Disarmed Load |
| **Hover Current ($I_{\text{hover}}$)** | `11.5 A` | Current at $1460\mu\text{s}$ Throttle |
| **Max Current ($I_{\text{max}}$)** | `45.0 A` | Current at $2000\mu\text{s}$ Throttle |
| **Update Rate** | `20 Hz` | 50ms Integration Loop |
