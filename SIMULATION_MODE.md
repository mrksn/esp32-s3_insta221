# Simulation Mode

## Overview

The simulation mode allows you to test the PID controller and SSR output without connecting real temperature sensors. This is useful for:

1. **Testing SSR PWM output** - Monitor GPIO 2 with an oscilloscope to verify the PID controller is generating correct PWM signals
2. **PID tuning verification** - Test PID parameters without risking damage to heating elements
3. **System development** - Develop and test control logic without full hardware setup

## How It Works

When simulation mode is enabled:
- **Temperature readings** come from a simulated heat plate model instead of the MAX31855 sensor
- **SSR output** on GPIO 2 still operates normally and can be monitored with an oscilloscope
- **Heat plate simulation** uses a thermal model that responds to SSR duty cycle

### Thermal Model

The simulation uses a first-order thermal model:

```
dT/dt = (P_heating - k * (T - T_ambient)) / C
```

Where:
- `T` = current temperature (°C)
- `P_heating` = heating power input (W) based on SSR PWM duty cycle
- `k` = heat loss coefficient (15 W/°C)
- `T_ambient` = ambient temperature (20°C)
- `C` = thermal mass (2200 J/°C)

**Parameters:**
- Maximum heating power: 2200W (matches real 2200W heating element)
- Thermal mass: 2200 J/°C (approximates aluminum heat plate)
- Heat loss: 15 W/°C (convection + radiation)

## Enabling Simulation Mode

### Option 1: Edit system_config.h

1. Open [`main/utils/system_config.h`](main/utils/system_config.h)
2. Find the simulation configuration section:
   ```c
   .simulation = {
       .enabled = false,  // Set to true to enable simulation
   },
   ```
3. Change `false` to `true`:
   ```c
   .simulation = {
       .enabled = true,  // Simulation mode enabled
   },
   ```
4. Rebuild and flash the firmware

### Option 2: Runtime Configuration (Future Enhancement)

In the future, this could be added as a menu option in the Settings menu for runtime switching.

## Using Simulation Mode

### 1. Enable Simulation Mode

Follow the steps above to enable simulation mode.

### 2. Connect Oscilloscope

Connect an oscilloscope probe to:
- **Signal**: GPIO 2 (SSR_PIN)
- **Ground**: Any GND pin on the ESP32-S3

### 3. Boot the System

When the system boots with simulation mode enabled, you'll see:

```
I (1234) sensors: Initializing in SIMULATION mode - no real hardware
I (1235) sensors: SSR output on GPIO 2 can be monitored with oscilloscope
```

### 4. Test PID Control

1. Navigate to **Heat Up** mode from the main menu
2. The system will use simulated temperature
3. Watch the oscilloscope to see PWM duty cycle changes
4. Expected behavior:
   - Initial heating: High duty cycle (approaching 100%)
   - Approaching target: Decreasing duty cycle
   - At target: Low duty cycle for maintenance (5-20%)

### 5. Monitor Simulation

The logs will show:
```
D (5678) sensors: Simulation temperature: 45.3°C (power: 75.2%)
D (5779) heating: Heating power set to 75% (duty: 767)
```

## Expected Oscilloscope Readings

### PWM Characteristics
- **Frequency**: 1 kHz (1ms period)
- **Logic levels**: 0V (low) to 3.3V (high)
- **Duty cycle range**: 0-100%

### Typical Control Behavior

1. **Cold start (20°C → 140°C)**:
   - First 30-60s: 80-100% duty cycle
   - Middle phase: Gradually decreasing from 80% to 40%
   - Final approach: 20-40% duty cycle
   - At target: 5-15% duty cycle (steady state)

2. **Steady state maintenance**:
   - Duty cycle oscillates around 10-20%
   - Frequency of oscillation depends on PID Ki term

## Thermal Model Behavior

### Heat Up Rate
Starting from 20°C with 100% power:
- Approximately **2-3°C per second** initially
- Slower as temperature increases (heat loss increases)
- Reaches 140°C in approximately **2-3 minutes**

### Cooling Rate
With heating off (0% power):
- Approximately **0.5-1°C per second** from 140°C
- Exponential decay toward ambient (20°C)

## Verification Tests

### Test 1: Manual Power Control
1. Set simulation mode enabled
2. In main.c, temporarily set fixed heating power:
   ```c
   heating_set_power(50);  // 50% duty cycle
   ```
3. Verify oscilloscope shows 50% duty cycle at 1 kHz
4. Check logs show temperature rising steadily

### Test 2: PID Response
1. Set target temperature to 140°C
2. Enter Heat Up mode
3. Observe:
   - Temperature rises from 20°C
   - PWM duty cycle decreases as target approaches
   - Temperature stabilizes at ~140°C
   - Minimal overshoot (< 5°C)

### Test 3: PID Tuning
1. Modify PID parameters in Settings
2. Enter Heat Up mode
3. Monitor oscilloscope for control changes
4. Verify smooth control without excessive oscillation

## Switching Back to Real Hardware

1. Open `main/utils/system_config.h`
2. Set simulation mode to false:
   ```c
   .simulation = {
       .enabled = false,  // Real hardware mode
   },
   ```
3. Rebuild and flash

## Troubleshooting

### No PWM output on oscilloscope
- Check GPIO 2 connection
- Verify probe ground connection
- Confirm heating switch is ON (simulation respects switch state)
- Check logs for heating warnings

### Temperature not changing
- Verify simulation mode is enabled (check boot logs)
- Check heating switch is enabled
- Look for `sensor_sim_set_heating_power()` calls in logs
- Verify PID controller is running (check main loop)

### Unrealistic behavior
- The thermal model is simplified
- Real heat plates have more complex thermal dynamics
- Use this for basic testing, validate with real hardware

## Code Structure

### Files Modified
1. **`main/utils/system_config.h`** - Added simulation configuration flag
2. **`components/sensors/sensors.c`** - Added thermal simulation model
3. **`components/heating/heating.c`** - Hooks simulation power updates

### Key Functions
- `sensor_sim_set_heating_power()` - Updates simulated heating power
- `sensor_sim_update_temperature()` - Updates thermal model
- `sensor_read_temperature()` - Returns simulated temp when enabled
- `sensor_init()` - Skips hardware init in simulation mode

## Notes

- SSR output (GPIO 2) is **always active** regardless of simulation mode
- This allows oscilloscope verification of PID control signals
- The simulation provides realistic thermal response for development
- Always perform final validation with real hardware before production use
