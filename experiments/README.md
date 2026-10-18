# Experiments

The robot's final controller hasn't arrived yet, so each subsystem is being proven on an **Arduino UNO Q** development board first. Like the robot, the UNO Q has a real-time microcontroller and a Linux processor. Code is written against a stable interface so it carries over to the production controller.

Every experiment folder follows the same layout:

```
README.md   question, setup, results, conclusions, how to reproduce
sketch/     microcontroller code
python/     Linux-side code
analysis/   offline analysis scripts
data/       raw logs: nothing is summarized away
results/    plots
```

| # | Experiment | Question | Key result | Status |
|---|---|---|---|---|
| 01 | **[IMU Bridge Latency](exp01-imu-bridge-latency/)** | How fast and how predictable is the real-time ↔ Linux link? | About 6 ms typical, 23 ms worst case, robust to full CPU load. Balance must stay on the MCU. | ✅ Complete |
| 02 | Voice command pipeline | Can the robot hear a command, decide and act end to end? | | 🔜 Planned |
| 03 | Vision and stereo cameras | Detection plus stereo depth for the head; link latency under vision load | | 🔜 Planned |
| 04 | Battery state-of-health | Track pack health and degradation over time | | 🔜 Planned |
| 05 | CAN-FD bring-up | Talk reliably to the joint actuators | | 🔜 Planned |
