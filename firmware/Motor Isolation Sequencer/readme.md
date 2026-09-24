## SEQUENTIAL MOTOR ISOLATION DIAGNOSTIC

1. SYSTEM OVERVIEW
------------------
The motor isolation sequencer provides deterministic verification of ESC pin routing, 
solder continuity, and motor rotation directions prior to closed-loop flight. 
Each ESC channel is energized individually at low throttle to eliminate multi-axis 
hardware ambiguity.

2. OPERATIONAL TIMING FORMULATION
---------------------------------
Each drive channel $M_k$ (where $k \in \{1, 2, 3, 4\}$) is actuated using a pulse 
width function defined over discrete step interval $t$:

$$u_{M_k}(t) = \begin{cases}  1150\mu\text{s} & \text{for } t_{\text{start}, k} \le t < t_{\text{start}, k} + 3.0\text{s} \\  1000\mu\text{s} & \text{otherwise}  \end{cases}$$

Where $u = 1150\mu\text{s}$ represents a safe low-idle rotational threshold.

3. DIAGNOSTIC MATRIX
--------------------


`| Sequence | Target Motor  | GPIO Pin  | Expected Rotation     |`



` Step 1     1 (Front-R)     GPIO 19     Counter-Clockwise(CCW)`

` Step 2     2 (Rear-R)      GPIO 14     Clockwise (CW)        `

` Step 3     3 (Rear-L)      GPIO 13     Counter-Clockwise(CCW)`

` Step 4     4 (Front-L)     GPIO 18     Clockwise (CW)        `



4. SAFETY VERIFICATION
----------------------
A 5.0-second delay occurs on boot before sequence execution. If any motor 
spins out of turn or in reverse direction, adjust physical phase wiring or 
re-check pin definitions in `include/config.h`.
