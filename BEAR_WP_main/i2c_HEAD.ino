/* =========================================================================
 *  i2c_HEAD.ino  (TAIL side - now SLAVE, listens for HEAD's writes)
 * -------------------------------------------------------------------------
 *  ROLE REVERSED - see i2c_slaveino.ino on HEAD for why. HEAD's I2C2_Bus
 *  turned out to already be committed to its INA228 current sensor as
 *  master, so it couldn't also run as a slave for this link without
 *  fighting that sensor for the same peripheral. HEAD is now master
 *  (reusing its existing Wire bus, already master to its IO extender);
 *  TAIL is now the slave, still using I2C2_Bus, just listening instead
 *  of polling.
 *
 *  UNCONFIRMED: assumes I2C2_SDA/I2C2_SCL (32/33) on this board actually
 *  connects to HEAD's Wire (I2C_SDA/I2C_SCL, 21/22) via the JST header -
 *  not HEAD's I2C2 as originally (and wrongly) assumed. The earlier
 *  "pin numbers happen to match" reasoning that pointed at I2C2 was
 *  built on a wrong premise about what HEAD's I2C2 was for - confirm
 *  this with an actual continuity check, not by pin numbers.
 * =========================================================================
 */

#define HEAD_LINK_ADDR 0x42  // must match TAIL_LINK_ADDR in HEAD's i2c_slaveino.ino exactly

// Must be byte-for-byte identical to HeadTelemetry in HEAD's i2c_slaveino.ino.
struct __attribute__((packed)) HeadTelemetry {
  float BV;
  float Current;
  float BatTemp;
  uint8_t heaterOn;
};

// Fires on I2C2_Bus's own interrupt context whenever HEAD writes to this
// address - keep this fast, no Serial prints beyond the error path, no
// delays. Updates BV/Current/BatTemp/heaterOn directly; nothing else in
// the sketch needs to poll for this data anymore.
void onHeadReceive(int len) {
  if (len != (int)sizeof(HeadTelemetry)) {
    Serial.printf("I2C link: got %d bytes from HEAD, expected %d\n", len, (int)sizeof(HeadTelemetry));
    while (I2C2_Bus.available()) I2C2_Bus.read();  // drain so it doesn't linger for the next transaction
    return;
  }
  HeadTelemetry t;
  I2C2_Bus.readBytes((uint8_t *)&t, sizeof(t));
  BV = t.BV;
  Current = t.Current;
  BatTemp = t.BatTemp;
  heaterOn = t.heaterOn != 0;
  // Raise the flag for the main loop
  
  newTelemetryReady = true;

}

void setup_head_link() {
  I2C2_Bus.onReceive(onHeadReceive);
  if (!I2C2_Bus.begin(HEAD_LINK_ADDR, I2C2_SDA, I2C2_SCL, 100000)) {
    Serial.println("I2C link: slave init failed");
    return;
  }
  Serial.printf("I2C link: slave up on 0x%02X (SDA=%d SCL=%d)\n", HEAD_LINK_ADDR, I2C2_SDA, I2C2_SCL);
}