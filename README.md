# Humanoid

### Built by Hand. Bound for Space.

A **5'9", 29-joint bipedal humanoid robot**, designed and built from scratch: no kit, no off-the-shelf frame. It's an anticipatory personal assistant here on Earth, and a step toward robotics for space.

**Michael J. Caruana**, independent robotics engineer
[mjcaruana.com](https://mjcaruana.com) · [GitHub](https://github.com/mobileluckey) · [Support the build](https://gofundme.com/u/michael-caruana)

---

## At a glance

| | |
|---|---|
| **Scale** | 5'9" (1.75 m), 29 actuated joints including articulated hands |
| **Structure** | 3D-printed carbon-fiber composite, designed in CAD |
| **Electronics** | Custom-designed circuit boards; distributed real-time controllers on a high-speed CAN-FD bus |
| **Intelligence** | LLM-based reasoning layer over deterministic real-time control; on-device learning |
| **Method** | Every architecture decision is backed by a measured, published experiment |

## Latest result: [Exp01, IMU Bridge Latency](experiments/exp01-imu-bridge-latency/)

Measured the link between the robot's real-time controller and its Linux "thinking" processor across three runs and 153,808 samples:

- **About 6 ms typical, up to about 23 ms worst case.** So balance and joint control must stay on the microcontroller.
- **Robust to load.** With the Linux CPU at 100%, the MCU dropped zero samples and typical latency rose only 0.4 ms.
- **Found and fixed a hardware-level sensor glitch** (an IMU register byte tear) and identified gimbal lock in the attitude filter, which drove the move to quaternions.

<p align="center"><img src="experiments/exp01-imu-bridge-latency/results/run3-load.png" width="600" alt="Exp01 run 3: latency, sample timing and attitude under full CPU load"></p>

## Repository guide

| Where | What |
|---|---|
| **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)** | The two-brain control design, and a log of design decisions with the evidence for each |
| **[docs/INNOVATION.md](docs/INNOVATION.md)** | The vision: an assistant that anticipates needs, with on-device, privacy-first learning |
| **[experiments/](experiments/)** | Subsystem experiments, each with code, raw data, analysis and conclusions |

## Experiments

| # | Experiment | Status |
|---|---|---|
| 01 | [IMU Bridge Latency](experiments/exp01-imu-bridge-latency/): real-time ↔ Linux link timing | ✅ Complete |
| 02 | Voice command pipeline: hear → decide → act | 🔜 Planned |
| 03 | Vision and stereo cameras for the head | 🔜 Planned |
| 04 | Battery state-of-health tracking | 🔜 Planned |
| 05 | CAN-FD actuator bring-up | 🔜 Planned |

## The vision

Most assistant robots wait for a command. This one is meant to **learn your routines and help before you ask**, with all learning on-device so your data stays yours.

> *"The best assistant is one you don't have to ask."*

[Read the full innovation statement →](docs/INNOVATION.md)

## Skills on display

- **Embedded and real-time:** C++ on STM32, fixed-rate control loops, sensor fusion, register-level I²C debugging
- **Systems:** two-processor architecture, Bridge RPC interface design, clock synchronization, latency and jitter analysis
- **Software and AI:** Python, data analysis (pandas, NumPy, matplotlib), LLM-agent and continuous-learning system design
- **Mechanical and electrical:** CAD, carbon-fiber additive manufacturing, PCB design, battery and power-system design
- **Method:** hypothesis → measurement → published raw data → design decision

---

*Follow the build at [mjcaruana.com](https://mjcaruana.com).*
