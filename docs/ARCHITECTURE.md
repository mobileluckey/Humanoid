# Architecture

## Two brains: reflex and deliberation

People don't think about keeping their balance. A reflex handles it, while the conscious mind plans what to do next. The robot is split the same way.

```
┌─────────────────────────────────────────────────────────────┐
│  DELIBERATION: Linux / AI processor                  10–50 Hz │
│  LLM agent · routine learning · vision · voice · planning    │
└──────────────────────────┬──────────────────────────────────┘
                           │  Bridge: commands down, state up
                           │  (measured: ~6 ms typical, ≤ ~23 ms worst)
┌──────────────────────────▼──────────────────────────────────┐
│  REFLEX: real-time microcontrollers               100–1,000 Hz │
│  balance · joint control · e-stop · power safety             │
└─────────────────────────────────────────────────────────────┘
```

**The rule:** anything with a deadline shorter than the Bridge's worst-case latency lives on the reflex side. The deliberation layer can *request* motion, but it never closes a safety-critical loop.

## Design decisions

Each decision links to the experiment that supports it.

| Decision | Why | Evidence |
|---|---|---|
| Balance, joint control and the e-stop run on the microcontroller | The Bridge adds 6–23 ms of variable delay, which is too much for a balance loop | [Exp01](../experiments/exp01-imu-bridge-latency/) |
| Linux can run at full load without endangering control | The MCU dropped 0 samples, and typical latency rose only 0.4 ms with all cores at 100% | [Exp01, run 3](../experiments/exp01-imu-bridge-latency/#run-3-linux-under-full-cpu-load-82-min-49204-samples-bdu-enabled) |
| The two processors sync clocks periodically | The MCU clock runs about 0.09% slow, and the rate shifts with temperature | [Exp01](../experiments/exp01-imu-bridge-latency/) |
| The attitude estimator uses quaternions, not Euler angles | Roll/pitch angles hit gimbal lock near 90°, which is exactly the regime of a fall | [Exp01, run 2](../experiments/exp01-imu-bridge-latency/#run-2-tilt-51-min-30443-samples) |
| IMU Block Data Update is always enabled | Without it, the gyro produced false ±15.6 °/s spikes | [Exp01](../experiments/exp01-imu-bridge-latency/) |
| The balance loop will use a hardware timer | Polling gives ±0.5 ms period jitter, which is fine at 100 Hz but not at 1 kHz | [Exp01](../experiments/exp01-imu-bridge-latency/) |

## Stable interfaces

Experiments use the same named functions across the Bridge (for example `ping`, `get_status` and sensor notifications) that the production robot will use. When the final controller arrives, only the code underneath those names should need to change.
