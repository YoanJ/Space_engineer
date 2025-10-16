# Yezus Printer Helper

Sensor auto-ranging utility for Space Engineers printer frames. The script scans the welders on a target grid and adjusts a sensor block so its detection box tightly wraps the working area, keeping printers responsive without manual retuning.

## Highlights
- Works with grids that move via pistons/rotors; optional auto-update keeps the sensor aligned.
- Supports either a named block group or simple name tags to identify welders.
- Applies configurable padding and respects the sensor's hardware range limits.
- Same-grid and same-construct filters avoid false positives from nearby contraptions.
- Mirrors the latest status log onto the programmable block's built-in LCD for quick checks.
- Debug echo stream helps verify the matched blocks and resulting extents (LCD shows the same output).

## Setup
- Place a programmable block on the printer controller grid and upload `script.c`.
- Add a sensor on the printer frame that should track your welders.
- Tag your welders either with a common substring (e.g. prefix/suffix) or add them to a block group.
- On the programmable block, paste the configuration template below into Custom Data and adjust values.
- Run the programmable block once (`Run` with no arguments) to confirm the sensor updates.

```ini
[SensorSetup]
SensorName       = [Printer Sensor]  ; Substring match for the target sensor
WelderGroup      =                   ; Optional block group containing the welders
WelderTag        = [Welder]          ; Substring tag if no block group is used
PaddingMeters    = 0.5               ; Extra clearance applied in every direction
AutoUpdate       = false             ; true -> recompute every 100 ticks
OnlySensorGrid   = true              ; Limit welders to the sensor's grid
SameConstruct    = true              ; Require welders on the same construct
Debug            = false             ; Echo extra diagnostics
```

## Runtime Commands
- `Run` (empty argument): Loads config, rebuilds the block cache if needed, and aligns the sensor.
- `Run` with `refresh` or `scan`: Forces a welder/sensor rescan before aligning.
- `Run` with `help`: Prints the built-in quick reference to the programmable block terminal.

## Tips
- Enable `AutoUpdate` when the printer geometry changes during operation (pistons, rotors, hinges). Otherwise leave it off to save script time.
- If multiple sensors share the same substring, tighten `SensorName` to avoid ambiguity.
- When the welders live on a sub-grid (e.g. rotor head), set `OnlySensorGrid = false` or `SameConstruct = false` as appropriate.
- Turn on `Debug` temporarily to inspect the matched welders list and measured extents via the programmable block console.
- After major printer modifications, rerun with `refresh` to rebuild the cache immediately.
