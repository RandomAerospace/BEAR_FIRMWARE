/* =========================================================================
 *  i2c_slaveino.ino  (HEAD side - MASTER, writes to TAIL)
 * -------------------------------------------------------------------------
 *  HEAD is master, using I2C2_Bus (32/33) - confirmed to be the bus
 *  physically wired to TAIL. That's the same peripheral the INA228
 *  current sensor already uses as master; TAIL (0x42) is just one more
 *  addressable device on it, same as any other I2C bus with multiple
 *  chips on it. No new peripheral, no slave mode on this side at all -
 *  HEAD only ever initiates.
 *
 *  Earlier versions of this file used Wire instead, based on a stale
 *  "i2c bridge" pin comment that didn't reflect the real wiring. Fixed.
 * =========================================================================
 */

#define TAIL_LINK_ADDR 0x42  // must match HEAD_LINK_ADDR in TAIL's i2c_HEAD.ino exactly

// Must be byte-for-byte identical to the struct in TAIL's i2c_HEAD.ino.
struct __attribute__((packed)) HeadTelemetry {
  float BV;
  float Current;
  float BatTemp;
  uint8_t heaterOn;
};

void setup_tail_link() {
  // No begin() here - I2C2_Bus is already initialized in setup() for the
  // INA228. Calling begin() a second time on it would risk the same
  // double-init problem that broke the earlier slave-mode version.
  Serial.printf("I2C link: master ready, will write to 0x%02X on I2C2_Bus\n", TAIL_LINK_ADDR);
}

// Call from loop() after BV/Current/BatTemp/heaterOn are updated for
// this cycle. Shares I2C2_Bus with the INA228 (both master, different
// addresses - not a conflict the way slave-mode sharing was). Returns
// false and logs on any failure; doesn't retry or block beyond the
// bus's own default transaction timeout.
bool send_head_telemetry() {
  HeadTelemetry t;
  t.BV = BV;
  t.Current = Current;
  t.BatTemp = BatTemp;
  t.heaterOn = heaterOn ? 1 : 0;

  I2C2_Bus.beginTransmission(TAIL_LINK_ADDR);
  I2C2_Bus.write((uint8_t *)&t, sizeof(t));
  uint8_t err = I2C2_Bus.endTransmission();
  if (err != 0) {
    // 0=success 1=data too long 2=NACK on address 3=NACK on data 4=other 5=timeout
    Serial.printf("I2C link: write to TAIL failed (err %d)\n", err);
    return false;
  }
  return true;
}
