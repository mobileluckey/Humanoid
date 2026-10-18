# Experiment 1 — IMU Bridge Latency (Linux / Python side)  v2
# Logs every IMU sample with a Linux arrival timestamp, and pings the MCU
# 5x per second to measure round-trip time and the clock offset.
# Output: logs/imu_<stamp>.csv and logs/ping_<stamp>.csv next to this script.

import csv
import time
from pathlib import Path
import os
os.environ["TZ"] = "America/Chicago"; time.tzset()

from arduino.app_utils import App, Bridge

LOG_DIR = Path(__file__).resolve().parent / "logs"
LOG_DIR.mkdir(exist_ok=True)
stamp = time.strftime("%Y%m%d-%H%M%S")

imu_f = open(LOG_DIR / f"imu_{stamp}.csv", "w", newline="")
ping_f = open(LOG_DIR / f"ping_{stamp}.csv", "w", newline="")
imu_w = csv.writer(imu_f)
ping_w = csv.writer(ping_f)
imu_w.writerow(["seq", "mcu_us", "linux_ns", "roll", "pitch", "gx", "gy", "gz"])
ping_w.writerow(["t0_ns", "t1_ns", "mcu_us"])

print(f"Logging to {LOG_DIR} (run {stamp})")

count = 0
last_roll = last_pitch = 0.0


def on_imu(seq, mcu_us, roll, pitch, gx, gy, gz):
    t = time.monotonic_ns()  # take the arrival time FIRST
    global count, last_roll, last_pitch
    imu_w.writerow([seq, mcu_us, t, roll, pitch, gx, gy, gz])
    count += 1
    last_roll, last_pitch = roll, pitch


Bridge.provide("imu", on_imu)

last_report = time.monotonic()
last_count = 0
ping_fail = 0
bdu_checked = False


def check_bdu():
    """Report once whether the sketch managed to enable Block Data Update."""
    try:
        s = int(Bridge.call("get_bdu_status"))
    except Exception as e:
        print(f"BDU status: could not ask the MCU ({e})")
        return
    if s == 0:
        print("BDU status: NOT SET - sensor not found on Wire1 or Wire")
    elif s < 0:
        print(f"BDU status: write did NOT stick (bus {-s // 1000}, addr 0x{-s % 1000:02X})")
    else:
        print(f"BDU status: ON (bus {s // 1000}, addr 0x{s % 1000:02X})")


def loop():
    global last_report, last_count, ping_fail, bdu_checked

    if not bdu_checked:
        check_bdu()
        bdu_checked = True

    t0 = time.monotonic_ns()
    try:
        mcu_us = Bridge.call("ping")
        t1 = time.monotonic_ns()
        ping_w.writerow([t0, t1, mcu_us])
    except Exception as e:
        ping_fail += 1
        print(f"ping failed: {e}")

    now = time.monotonic()
    if now - last_report >= 5.0:
        rate = (count - last_count) / (now - last_report)
        imu_f.flush()
        ping_f.flush()
        print(f"{rate:6.1f} Hz | roll {last_roll:+6.1f}  pitch {last_pitch:+6.1f} "
              f"| samples {count} | ping fails {ping_fail}")
        last_report, last_count = now, count

    time.sleep(0.2)


App.run(user_loop=loop)
