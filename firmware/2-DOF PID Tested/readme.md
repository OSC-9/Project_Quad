## 250Hz DUAL-AXIS CASCADE PID CONTROLLER

1. ARCHITECTURAL OVERVIEW
-------------------------
The flight controller uses a dual-axis (pitch/roll) cascade control structure 
running in a hard real-time loop at 250Hz ($\Delta t = 0.004\text{s}$)[cite: 18]. The outer 
loop processes absolute attitude angles (Degrees), generating target angular velocities 
for the inner loop (Degrees/Second) to command motor PWM outputs[cite: 18].

2. CASCADE PID MATHEMATICAL MODEL
---------------------------------

OUTER ANGLE CONTROL LOOP (P-Dominant):
$$e_{\theta}(t) = \theta_{\text{setpoint}}(t) - \theta_{\text{estimated}}(t)$$
$$\omega_{\text{target}}(t) = K_{p,\theta} \cdot e_{\theta}(t) + K_{i,\theta} \int e_{\theta}(t) dt + K_{d,\theta} \frac{d e_{\theta}(t)}{dt}$$

INNER RATE CONTROL LOOP (Full PID):
$$e_{\omega}(t) = \omega_{\text{target}}(t) - \omega_{\text{gyro}}(t)$$
$$u_{\text{axis}}(t) = K_{p,\omega} \cdot e_{\omega}(t) + K_{i,\omega} \int e_{\omega}(t) dt + K_{d,\omega} \frac{d e_{\omega}(t)}{dt}$$

3. QUADCOPTER X-FRAME MIXING MATRIX
-----------------------------------
The combined axis outputs are blended with auto-spool throttle $T_{\text{auto}}$ to produce 
individual motor command pulses:

$$\begin{bmatrix} M_{\text{FL}} \\ M_{\text{FR}} \\ M_{\text{BL}} \\ M_{\text{BR}} \end{bmatrix} = \begin{bmatrix} 1 & 1 & 1 \\ 1 & 1 & -1 \\ 1 & -1 & 1 \\ 1 & -1 & -1 \end{bmatrix} \begin{bmatrix} T_{\text{auto}} \\ u_{\text{pitch}} \\ u_{\text{roll}} \end{bmatrix}$$

Output Constraint Enforcer:
$$M_k = \text{clamp}(M_k, 1000\mu\text{s}, 1850\mu\text{s})$$

4. TIMING GUARANTEES
--------------------
Loop frequency is maintained via a hardware microsecond timer lock[cite: 18]:

$$\Delta t_{\text{execution}} = t_{\text{current}} - t_{\text{previous}} = 4000\mu\text{s}$$

During spin-wait states, incoming SBUS frame buffers are polled over hardware serial[cite: 18].
