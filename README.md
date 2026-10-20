# BLDC Motor Controller (Arduino, PI Control and Custom PCB)

Sensored 6-step BLDC motor control built around an Arduino and three IR2112 half-bridge gate drivers. It covers open-loop speed control, closed-loop PI speed control, hardware overcurrent protection and a KiCad PCB layout. The simulation was done in Proteus.

| Stage | What it does | Tools |
|---|---|---|
| **PS-1** | Open-loop Hall-sensor commutation, CW/CCW switching, overcurrent cutoff | Proteus, Arduino |
| **PS-2** | Closed-loop PI speed control (200–500 RPM) | Proteus, Arduino |
| **Bonus** | 2-layer power-stage PCB with current-rated traces | KiCad |

## Hardware

- **Power stage:** 3 half-bridges (6 N-MOSFETs) driven by 3x IR2112. Each phase has a bootstrap diode and capacitor so the high-side gate can be driven to VS + 12 V.
- **Sensing:** 3 Hall sensors with 1k pull-ups give rotor position.
- **Overcurrent protection:** a single 0.1 Ω shunt on the shared low-side return feeds a comparator. Only 2 phases conduct at a time, so one shunt sees the total current. The comparator trips an Arduino input, which turns every MOSFET off.
- **Controls:** a potentiometer sets speed and a button toggles direction.

## PS-1: Open-Loop Control

- Found the Hall code for each of the 6 commutation steps experimentally. I forced each step, read the Hall bits, and noticed that the two values seen per step correspond to CW vs CCW.
- Added a manual 6-step startup kick, because the motor can't self-start from Hall feedback alone.
- Mapped the pot to the delay between commutation steps. This targeted RPM much more predictably than PWM duty did, with slight oscillation about the target.
- Overcurrent trips immediately and all gates go low. RPM then coasts down slowly because the simulation has little friction.

## PS-2: Closed-Loop PI Control

- The pot sets a target of 200–500 RPM. Measured RPM is computed from the time between Hall transitions, and the PI controller adjusts PWM duty on the high-side switches.
- Hall transitions stood in for a rotary encoder, since Proteus has no encoder compatible with this motor model. This gives only 6 updates per electrical revolution, so resolution is lower.
- Integral control reduces steady-state error.
- The accumulated error is capped to prevent overshoot.
- Kp = 2, Ki = 1.2, tuned by hand along with motor friction and load.

| Target | Overshoot | Settled at |
|---|---|---|
| 200 RPM | ~15 RPM | ~190–195 RPM |
| 500 RPM | none | ~490 RPM |

The residual steady-state error is a deliberate trade-off. Raising Ki further caused overshoot, oscillation and Proteus "timestep too small" errors. Less overshoot at 500 RPM is likely because friction rises with speed.

## Bonus: PCB (KiCad)

- Reused the PS-1 power and comparator schematic. The motor, Hall sensors and button are brought out to headers, and the comparator Vref comes from a fixed voltage divider instead of a pot.
- Connectors: J1 (+12 V / +5 V), J2 (3-phase motor output), J3 (8-pin Arduino interface).
- Ran ERC (added PWR_FLAGs and the LM393 supply pins), assigned footprints, and routed the board in 2 layers on 80 x 80 mm.
- **Trace sizing:** the sustained current measured in simulation was ~7 A, which needs ~2.2 mm width at a 10 °C rise using the KiCad calculator. 1 oz copper made this too wide to route, so I switched to **2 oz (70 µm)**. Signal traces use the default width.
- A ~8 A transient spike was ignored for sizing, since trace heating depends on sustained rather than instantaneous current.
- DRC passed and the board was checked in 3D view.

## Known Limitations

- Simulation only, no hardware build or physical testing.
- Tuning is specific to the 200–500 RPM range and the load and friction I set in Proteus.
- Single pole-pair motor assumed in the RPM calculation (`POLE_PAIRS = 1`).
- Footprints for PCB were chosen from online research and not checked against real component availability.
