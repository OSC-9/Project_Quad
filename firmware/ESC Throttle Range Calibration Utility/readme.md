## ESC THROTTLE RANGE CALIBRATOR

1. SYSTEM OVERVIEW
------------------
The Electronic Speed Controller (ESC) range calibration tool maps the digital 
Pulse Width Modulation (PWM) duty cycle produced by the microcontroller to the 
physical motor speed controllers. Analog and SimonK ESCs require explicit 
endpoint initialization to eliminate dead-band regions at minimum throttle and 
prevent clipping at maximum throttle.

2. MATHEMATICAL SIGNAL SPECIFICATION
------------------------------------
The control signal $u(t)$ sent to each ESC output channel is bounded within standard 
servo pulse-width constraints:

$$u(t) \in [1000\mu\text{s}, 2000\mu\text{s}]$$

Where:
  * u_{min} = 1000 \mu\text{s} : Zero-throttle / Motor Arming Baseline
  * u_{max} = 2000 \mu\text{s} : Maximum Duty Cycle / High Calibration Setpoint
  * Pulse Frequency f = 50\text{Hz} (\text{Period } T = 20\text{ms})

3. HARDWARE OPERATIONAL PROTOCOL
--------------------------------
> WARNING: Remove all propellers before running calibration routines.

1. Disconnect the main LiPo power harness.
2. Flash `01_esc_calibration.ino` and launch Serial Monitor at 115200 baud.
3. Send string command `MAX` to force outputs to $2000\mu\text{s}$.
4. Connect the main LiPo battery. Wait for the ESC double-tone acknowledgement (`BEEP-BEEP`).
5. Send string command `MIN` to shift outputs to $1000\mu\text{s}$.
6. Listen for the confirmation chime indicating successful EEPROM timing writes.

4. HARDWARE PINOUT REFERENCE
----------------------------
  * Motor 1 (Front Right) : GPIO 19
  * Motor 2 (Rear Right)  : GPIO 14
  * Motor 3 (Rear Left)   : GPIO 13
  * Motor 4 (Front Left)  : GPIO 18
