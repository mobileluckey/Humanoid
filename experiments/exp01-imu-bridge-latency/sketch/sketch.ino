// Experiment 1 — IMU Bridge Latency (MCU side)  v2: adds Block Data Update fix
// Reads the Modulino Movement at a fixed 100 Hz, runs a complementary
// filter for roll/pitch, and streams timestamped samples to Python.
// Also exposes ping() so Python can measure round-trip time and
// estimate the offset between the MCU and Linux clocks.
//
// KEEP THE BOARD STILL for ~2 s after the sketch starts (gyro bias calibration).

#include <Arduino_RouterBridge.h>
#include <Arduino_Modulino.h>
#include <Wire.h>

ModulinoMovement movement;

const uint32_t PERIOD_US = 10000;   // 100 Hz loop
const float    ALPHA     = 0.98f;   // complementary filter: gyro weight

float gbx = 0, gby = 0, gbz = 0;    // gyro bias (deg/s)
float roll = 0, pitch = 0;          // attitude (deg)
uint32_t seq = 0;
uint32_t nextT = 0, lastT = 0;
uint32_t staleCount = 0;            // cycles where the IMU had no new data

// ---- Block Data Update (BDU) fix ----
// Without BDU, the sensor can update between reading the low and high byte
// of a gyro value, producing one-sample spikes of exactly 256 LSB (~15.6 dps).
// BDU makes the sensor hold both bytes until both have been read.
const uint8_t LSM6DSOX_WHO_AM_I = 0x0F;   // should read 0x6C
const uint8_t LSM6DSOX_CTRL3_C  = 0x12;   // bit 6 = BDU
long bduStatus = 0;   // 0 = sensor not found; otherwise bus*1000 + addr, negative if write didn't stick

int readReg(TwoWire &bus, uint8_t addr, uint8_t reg) {
  bus.beginTransmission(addr);
  bus.write(reg);
  if (bus.endTransmission(false) != 0) return -1;
  if (bus.requestFrom(addr, (uint8_t)1) != 1) return -1;
  return bus.read();
}

bool writeReg(TwoWire &bus, uint8_t addr, uint8_t reg, uint8_t val) {
  bus.beginTransmission(addr);
  bus.write(reg);
  bus.write(val);
  return bus.endTransmission() == 0;
}

// Look for the LSM6DSOX on the given bus at either address, then set BDU.
bool enableBDU(TwoWire &bus, int busId) {
  const uint8_t addrs[] = {0x6A, 0x6B};
  for (uint8_t a : addrs) {
    if (readReg(bus, a, LSM6DSOX_WHO_AM_I) != 0x6C) continue;
    int c3 = readReg(bus, a, LSM6DSOX_CTRL3_C);
    if (c3 < 0) continue;
    writeReg(bus, a, LSM6DSOX_CTRL3_C, (uint8_t)c3 | 0x40);
    int check = readReg(bus, a, LSM6DSOX_CTRL3_C);
    bduStatus = (check >= 0 && (check & 0x40)) ? (busId * 1000L + a) : -(busId * 1000L + a);
    return true;
  }
  return false;
}

// ---- Bridge-exposed functions (keep these names stable: they're the contract) ----
unsigned long ping() { return micros(); }
unsigned long get_stale_count() { return staleCount; }
long get_bdu_status() { return bduStatus; }

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

  // The Qwiic port may be on Wire1 or Wire depending on the core; try both.
  // If this fails to compile ("Wire1 was not declared"), change the line to:
  //   enableBDU(Wire, 0);
  if (!enableBDU(Wire1, 1)) enableBDU(Wire, 0);

  Bridge.provide("ping", ping);
  Bridge.provide("get_stale_count", get_stale_count);
  Bridge.provide("get_bdu_status", get_bdu_status);

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
