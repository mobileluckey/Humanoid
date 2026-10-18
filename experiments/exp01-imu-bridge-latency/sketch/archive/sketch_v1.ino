// Experiment 1 — IMU attitude + Bridge latency (MCU side)
// Reads the Modulino Movement at a fixed 100 Hz, runs a complementary
// filter for roll/pitch, and streams timestamped samples to Python.
// Also exposes ping() so Python can measure round-trip time and
// estimate the offset between the MCU and Linux clocks.
//
// KEEP THE BOARD STILL for ~2 s after the sketch starts (gyro bias calibration).

#include <Arduino_RouterBridge.h>
#include <Arduino_Modulino.h>

ModulinoMovement movement;

const uint32_t PERIOD_US = 10000;   // 100 Hz loop
const float    ALPHA     = 0.98f;   // complementary filter: gyro weight

float gbx = 0, gby = 0, gbz = 0;    // gyro bias (deg/s)
float roll = 0, pitch = 0;          // attitude (deg)
uint32_t seq = 0;
uint32_t nextT = 0, lastT = 0;
uint32_t staleCount = 0;            // cycles where the IMU had no new data

// ---- Bridge-exposed functions (keep these names stable: they're the contract) ----
unsigned long ping() { return micros(); }
unsigned long get_stale_count() { return staleCount; }

void calibrateGyro() {
  const int N = 300;
  float sx = 0, sy = 0, sz = 0;
  int n = 0;
  while (n < N) {
    if (movement.update()) {
      sx += movement.getRoll();    // gyro X (deg/s)
      sy += movement.getPitch();   // gyro Y
      sz += movement.getYaw();     // gyro Z
      n++;
    }
    delay(5);
  }
  gbx = sx / N; gby = sy / N; gbz = sz / N;
}

void initAttitudeFromAccel() {
  while (!movement.update()) delay(2);
  float ax = movement.getX(), ay = movement.getY(), az = movement.getZ();
  roll  = atan2f(ay, az) * RAD_TO_DEG;
  pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * RAD_TO_DEG;
}

void setup() {
  Bridge.begin();
  Modulino.begin();
  movement.begin();

  Bridge.provide("ping", ping);
  Bridge.provide("get_stale_count", get_stale_count);

  calibrateGyro();
  initAttitudeFromAccel();

  lastT = micros();
  nextT = lastT + PERIOD_US;
}

void loop() {
  uint32_t now = micros();
  if ((int32_t)(now - nextT) < 0) return;       // not time yet

  // If we fell far behind (e.g. a long Bridge stall), resync instead of bursting
  if ((int32_t)(now - nextT) > (int32_t)(5 * PERIOD_US)) nextT = now;
  nextT += PERIOD_US;

  if (!movement.update()) { staleCount++; return; }

  float dt = (now - lastT) * 1e-6f;
  lastT = now;

  float ax = movement.getX(), ay = movement.getY(), az = movement.getZ();   // g
  float gx = movement.getRoll()  - gbx;                                      // deg/s
  float gy = movement.getPitch() - gby;
  float gz = movement.getYaw()   - gbz;

  float rollAcc  = atan2f(ay, az) * RAD_TO_DEG;
  float pitchAcc = atan2f(-ax, sqrtf(ay * ay + az * az)) * RAD_TO_DEG;

  roll  = ALPHA * (roll  + gx * dt) + (1.0f - ALPHA) * rollAcc;
  pitch = ALPHA * (pitch + gy * dt) + (1.0f - ALPHA) * pitchAcc;

  // Timestamp is taken at the top of the cycle, before the IMU read
  Bridge.notify("imu", seq++, now, roll, pitch, gx, gy, gz);
}
