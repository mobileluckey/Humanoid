"""Experiment 1 analysis — run on your PC.

Usage:
    pip install pandas numpy matplotlib
    python analyze.py imu_<stamp>.csv ping_<stamp>.csv

What it reports:
  * MCU sample rate and loop jitter (from MCU timestamps)
  * Dropped samples (sequence gaps)
  * Bridge round-trip time (from pings)
  * One-way MCU -> Linux latency for IMU notifies, after estimating the
    clock offset/drift from the lowest-RTT pings (accuracy ~ +/- min RTT / 2)
  * Roll/pitch drift while still
Saves exp1_report.png next to the CSVs.
"""
import sys
from pathlib import Path

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

WRAP = 2**32


def unwrap_u32(x):
    x = np.asarray(x, dtype=np.int64)
    d = np.diff(x)
    jumps = np.concatenate([[0], np.cumsum(np.where(d < -WRAP // 2, WRAP, 0))])
    return x + jumps


def pct(a, p):
    return np.percentile(a, p)


def main(imu_path, ping_path):
    imu = pd.read_csv(imu_path)
    ping = pd.read_csv(ping_path)

    imu_mcu = unwrap_u32(imu.mcu_us)
    ping_mcu = unwrap_u32(ping.mcu_us)
    # Put both on the same wrap epoch
    shift = np.round((ping_mcu[0] - imu_mcu[0]) / WRAP) * WRAP
    ping_mcu = ping_mcu - shift

    # ---- MCU timing ----
    dt_us = np.diff(imu_mcu)
    gaps = np.diff(imu.seq.values) - 1
    dropped = int(gaps[gaps > 0].sum())

    # ---- Round trip ----
    rtt_us = (ping.t1_ns - ping.t0_ns).values / 1e3

    # ---- Clock offset (linux_us - mcu_us), fit with drift, best pings only ----
    mid_us = (ping.t0_ns + ping.t1_ns).values / 2e3
    off = mid_us - ping_mcu
    good = rtt_us <= pct(rtt_us, 20)
    b, a = np.polyfit(ping_mcu[good], off[good], 1)   # off = a + b*mcu
    drift_ppm = b * 1e6

    # ---- One-way latency of IMU notifies ----
    arrival_us = imu.linux_ns.values / 1e3
    lat_us = arrival_us - (imu_mcu + a + b * imu_mcu)

    # ---- Attitude drift while still ----
    dur_s = (imu_mcu[-1] - imu_mcu[0]) / 1e6

    print(f"\nRun length: {dur_s:.1f} s, {len(imu)} samples, {len(ping)} pings")
    print("\n-- MCU loop --")
    print(f"  rate        {1e6 / dt_us.mean():7.2f} Hz")
    print(f"  interval    mean {dt_us.mean():.0f} us, std {dt_us.std():.0f} us, max {dt_us.max():.0f} us")
    print(f"  dropped     {dropped} ({100 * dropped / (len(imu) + dropped):.2f} %)")
    print("\n-- Bridge round trip (ping) --")
    print(f"  min {rtt_us.min()/1e3:.2f}  p50 {pct(rtt_us,50)/1e3:.2f}  p95 {pct(rtt_us,95)/1e3:.2f}  "
          f"p99 {pct(rtt_us,99)/1e3:.2f}  max {rtt_us.max()/1e3:.2f} ms")
    print("\n-- MCU -> Linux notify latency --")
    print(f"  p50 {pct(lat_us,50)/1e3:.2f}  p95 {pct(lat_us,95)/1e3:.2f}  p99 {pct(lat_us,99)/1e3:.2f}  "
          f"max {lat_us.max()/1e3:.2f} ms")
    print(f"  jitter (p99 - p50) {(pct(lat_us,99) - pct(lat_us,50))/1e3:.2f} ms")
    print(f"  offset accuracy ~ +/- {rtt_us.min()/2e3:.2f} ms, clock drift {drift_ppm:+.1f} ppm")
    print("\n-- Attitude --")
    print(f"  roll  start {imu.roll.iloc[0]:+.2f}  end {imu.roll.iloc[-1]:+.2f}  std {imu.roll.std():.3f} deg")
    print(f"  pitch start {imu.pitch.iloc[0]:+.2f}  end {imu.pitch.iloc[-1]:+.2f}  std {imu.pitch.std():.3f} deg")

    # ---- Plots ----
    t = (imu_mcu - imu_mcu[0]) / 1e6
    fig, ax = plt.subplots(4, 1, figsize=(10, 12))
    ax[0].plot(t, lat_us / 1e3, ",")
    ax[0].set_title("MCU -> Linux notify latency"); ax[0].set_ylabel("ms")
    ax[1].hist(lat_us / 1e3, bins=200, log=True)
    ax[1].set_title("Latency histogram (log count)"); ax[1].set_xlabel("ms")
    ax[2].plot(t[1:], dt_us, ",")
    ax[2].set_title("MCU sample interval"); ax[2].set_ylabel("us")
    ax[3].plot(t, imu.roll, label="roll"); ax[3].plot(t, imu.pitch, label="pitch")
    ax[3].set_title("Attitude"); ax[3].set_ylabel("deg"); ax[3].set_xlabel("s"); ax[3].legend()
    fig.tight_layout()
    out = Path(imu_path).with_name("exp1_report.png")
    fig.savefig(out, dpi=120)
    print(f"\nSaved {out}")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2])
