# PID Parameters for 2200W/230V Heat Press

## Overview

This document explains the PID parameter choices for the Insta Retrofit heat press automation system with a 2200W/230V heating element and high thermal mass.

## Default PID Parameters

The system now uses the following default PID parameters:

```c
Kp = 3.5   // Proportional gain
Ki = 0.05  // Integral gain
Kd = 1.2   // Derivative gain
```

## Why These Values?

### High Thermal Mass Considerations

A heat press with high thermal mass has these characteristics:
- **Slow response time**: Takes longer to heat up and cool down
- **Large thermal inertia**: Resists temperature changes
- **Risk of overshoot**: Without proper tuning, can overshoot target temperature

### Parameter Rationale

#### Kp = 3.5 (Higher Proportional Gain)
- **Increased from 2.0 to 3.5**
- Higher Kp provides stronger response to temperature error
- Helps overcome the thermal inertia of the large heating mass
- Provides faster initial temperature rise
- Still conservative enough to prevent instability

#### Ki = 0.05 (Lower Integral Gain)
- **Decreased from 0.1 to 0.05**
- Lower Ki is crucial for high thermal mass systems
- Prevents integral windup that causes overshoot
- Allows system to slowly eliminate steady-state error
- Conservative value ensures temperature stability

#### Kd = 1.2 (Higher Derivative Gain)
- **Increased from 0.05 to 1.2**
- Higher Kd acts as a "brake" against rapid temperature changes
- Dampens oscillations around setpoint
- Predicts future error based on rate of change
- Essential for smooth temperature control with high thermal mass

## Performance Characteristics

With these parameters, you can expect:

1. **Heating Phase**:
   - Moderate speed to target temperature
   - Minimal overshoot (< 2-3°C)
   - Smooth approach to setpoint

2. **Steady State**:
   - Stable temperature control
   - Small oscillations (< 1°C)
   - Good disturbance rejection

3. **Safety**:
   - Conservative tuning prevents dangerous overshoot
   - System remains stable under varying load conditions

## Optimization with Auto-Tune

While these defaults provide safe and stable operation, **we strongly recommend using the auto-tune feature** to optimize PID parameters for your specific heat press:

### How to Run Auto-Tune

1. Navigate to: **Main Menu → Settings → Auto-Tune PID**
2. Press confirm to start
3. Wait 20-30 minutes for the process to complete
4. New parameters will be automatically saved and applied

### Auto-Tune Benefits

- **Optimal performance**: Tailored to your exact heat press characteristics
- **Relay feedback method**: Industry-standard Åström-Hägglund algorithm
- **Tyreus-Luyben tuning rule**: Conservative tuning with minimal overshoot
- **No guesswork**: Scientific measurement of system dynamics

### Auto-Tune Process

The auto-tune performs these steps:

1. Heats the system in relay feedback mode (on/off cycling)
2. Measures temperature oscillations over multiple cycles
3. Calculates ultimate gain (Ku) and ultimate period (Tu)
4. Applies Tyreus-Luyben tuning rule to calculate Kp, Ki, Kd
5. Saves and applies new parameters automatically

## Comparison with Original Parameters

| Parameter | Original | New Default | Rationale |
|-----------|----------|-------------|-----------|
| Kp | 2.0 | 3.5 | Better response to thermal mass |
| Ki | 0.1 | 0.05 | Prevent overshoot with high inertia |
| Kd | 0.05 | 1.2 | Dampen oscillations |

## Troubleshooting

### Temperature Overshoots Target
- Decrease Kp by 0.5
- Decrease Ki by 0.01
- Increase Kd by 0.2

### Temperature Rises Too Slowly
- Increase Kp by 0.5
- Keep Ki low to prevent overshoot
- Adjust Kd proportionally

### Temperature Oscillates at Setpoint
- Decrease Kp by 0.5
- Decrease Ki by 0.01
- Increase Kd by 0.3

### Best Solution
**Run auto-tune!** It will find the optimal parameters automatically.

## Technical Background

### Power Characteristics
- **Power**: 2200W at 230V = 9.57A
- **Heat capacity**: Large thermal mass requires significant energy input
- **Time constant**: Estimated τ ≈ 60-120 seconds (slow system)

### Control Strategy
- **PWM control**: 0-100% duty cycle
- **Hysteresis**: ±5°C band to prevent rapid cycling
- **Safety limits**: 220°C maximum, emergency shutdown

### Tuning Philosophy
- **Conservative by design**: Safety first
- **Stability over speed**: Prefer stable control to fast response
- **Overshoot prevention**: High thermal mass can be dangerous if overheated

## References

- Åström, K. J., & Hägglund, T. (1984). "Automatic Tuning of Simple Regulators"
- Ziegler, J. G., & Nichols, N. B. (1942). "Optimum Settings for Automatic Controllers"
- Tyreus, B. D., & Luyben, W. L. (1992). "Tuning PI Controllers for Integrator/Dead Time Processes"

## Conclusion

The new default PID parameters (Kp=3.5, Ki=0.05, Kd=1.2) are specifically optimized for a 2200W/230V heat press with high thermal mass. These values provide safe, stable operation with minimal overshoot.

**For best performance, run the auto-tune feature to customize parameters to your specific heat press.**
