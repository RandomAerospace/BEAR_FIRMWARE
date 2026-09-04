inline void generate_fskid_str(String s) {
  for (uint8_t i = 0; i < s.length(); i++) {
    generate_fskid_char(s.c_str()[i] - 0x20);
  }
}

inline void generate_fskid_char(uint8_t c) {
  uint8_t i = 0b1;
  while (i & 0b111111) {
    if (c & i) {
      set_dac_freq(1900);
    } else {
      set_dac_freq(2100);
    }
    delay(22);
    i <<= 1;
  }
}

inline uint8_t colour_lookup(uint8_t val, uint8_t comp) {
  if (comp < 3) {
    return im_cm[val][2 - comp];
  } else {
    return im_cm[val][3];
  }
}


#ifdef HW_ESP32S3
#define ROBOT36_YSCAN_DELAY (269)
#define ROBOT36_CSCAN_DELAY (268)
#else
// At 240MHz
//#define ROBOT36_YSCAN_DELAY (258)
//#define ROBOT36_CSCAN_DELAY (260)
// At 80MHz
#define ROBOT36_YSCAN_DELAY (255)
#define ROBOT36_CSCAN_DELAY (249)
#endif
// Pure audio/image encode. Tuning, PTT and settle are owned by
// sstv_transmit_frame() now, not by this function - it's called once,
// with the radio already tuned to 145.5500 and PTT already keyed and
// settled.
//
// This still runs start-to-finish in a single blocking call: the per-row/
// per-pixel timing below is held to single-digit-microsecond precision
// (delayMicroseconds), which a millis()-resolution state machine can't
// reproduce. That mirrors how RTTY_TRANSMITTING calls send_report() as one
// blocking call per APRS packet - the state machine handles *scheduling*
// around the transmission, not the sample-level timing inside it.
void robot_img(const uint8_t image[HEIGHT][WIDTH][COMPONENTS]) {
  //wake_dra818();
  tune_dra818("145.6700");
  digitalWrite(PIN_RAD_PTT, LOW);
  delay(500); 
  // VOX Signal
  set_dac_freq(1900);
  dac_cw_generator_enable();
  delay(100);
  set_dac_freq(1500);
  delay(100);
  set_dac_freq(1900);
  delay(100);
  set_dac_freq(1500);
  delay(100);
  set_dac_freq(2300);
  delay(100);
  set_dac_freq(1500);
  delay(100);
  set_dac_freq(2300);
  delay(100);
  set_dac_freq(1500);
  delay(100);

  // VIS + Sync
  set_dac_freq(1900);
  delay(300);
  set_dac_freq(1200);
  delayMicroseconds(10000);
  set_dac_freq(1900);
  delay(300);
  set_dac_freq(1200);
  delayMicroseconds(30000);
  // Robot32
  set_dac_freq(1300);
  delayMicroseconds(30000);
  set_dac_freq(1300);
  delayMicroseconds(30000);
  set_dac_freq(1300);
  delayMicroseconds(30000);
  set_dac_freq(1100);
  delayMicroseconds(30000);
  set_dac_freq(1300);
  delayMicroseconds(30000);
  set_dac_freq(1300);
  delayMicroseconds(30000);
  set_dac_freq(1300);
  delayMicroseconds(30000);
  set_dac_freq(1100);
  delayMicroseconds(30000);
  set_dac_freq(1200);
  delayMicroseconds(30000);

  // Image
  for (uint8_t i = 0; i < HEIGHT; i += 2) {
    set_dac_freq(1200);
    delayMicroseconds(9000);
    set_dac_freq(1500);
    delayMicroseconds(3000);
    for (uint16_t j = 0; j < WIDTH; j++) {
      float freq = 1500.0 + ((16.0 + (0.003906 * ((65.738 * colour_lookup(image[i][j][0], 0)) + (129.057 * colour_lookup(image[i][j][0], 1)) + (25.064 * colour_lookup(image[i][j][0], 2))))) * 3.1372549);
      set_dac_freq(freq);
      delayMicroseconds(ROBOT36_YSCAN_DELAY); // Ideal: 275
    }
    set_dac_freq(1500);
    delayMicroseconds(4500);
    set_dac_freq(1900);
    delayMicroseconds(1500);
    for (uint16_t j = 0; j < HALFWIDTH; j++) {
      const float hi0 =
        (float)((uint16_t)colour_lookup(image[i][2 * j][0], 0) +
                (uint16_t)colour_lookup(image[i + 1][2 * j][0], 0) +
                (uint16_t)colour_lookup(image[i][2 * j + 1][0], 0) +
                (uint16_t)colour_lookup(image[i + 1][2 * j + 1][0], 0)) / 4.0;
      const float hi1 =
        (float)((uint16_t)colour_lookup(image[i][2 * j][0], 1) +
                (uint16_t)colour_lookup(image[i + 1][2 * j][0], 1) +
                (uint16_t)colour_lookup(image[i][2 * j + 1][0], 1) +
                (uint16_t)colour_lookup(image[i + 1][2 * j + 1][0], 1)) / 4.0;
      const float hi2 =
        (float)((uint16_t)colour_lookup(image[i][2 * j][0], 2) +
                (uint16_t)colour_lookup(image[i + 1][2 * j][0], 2) +
                (uint16_t)colour_lookup(image[i][2 * j + 1][0], 2) +
                (uint16_t)colour_lookup(image[i + 1][2 * j + 1][0], 2)) / 4.0;
      float freq = 1500.0 + ((128.0 + (0.003906 * ((112.439 * hi0) + (-94.154 * hi1) + (-18.285 * hi2)))) * 3.1372549);
      set_dac_freq(freq);
      delayMicroseconds(ROBOT36_CSCAN_DELAY); // Ideal: 275
    }

    set_dac_freq(1200);
    delayMicroseconds(9000);
    set_dac_freq(1500);
    delayMicroseconds(3000);
    for (uint16_t j = 0; j < WIDTH; j++) {
      float freq = 1500.0 + ((16.0 + (0.003906 * ((65.738 * colour_lookup(image[i + 1][j][0], 0)) + (129.057 * colour_lookup(image[i + 1][j][0], 1)) + (25.064 * colour_lookup(image[i + 1][j][0], 2))))) * 3.1372549);
      set_dac_freq(freq);
      delayMicroseconds(ROBOT36_YSCAN_DELAY); // Ideal: 275
    }
    set_dac_freq(2300);
    delayMicroseconds(4500);
    set_dac_freq(1900);
    delayMicroseconds(1500);
    for (uint16_t j = 0; j < HALFWIDTH; j++) {
      const float hi0 =
        (float)((uint16_t)colour_lookup(image[i][2 * j][0], 0) +
                (uint16_t)colour_lookup(image[i + 1][2 * j][0], 0) +
                (uint16_t)colour_lookup(image[i][2 * j + 1][0], 0) +
                (uint16_t)colour_lookup(image[i + 1][2 * j + 1][0], 0)) / 4.0;
      const float hi1 =
        (float)((uint16_t)colour_lookup(image[i][2 * j][0], 1) +
                (uint16_t)colour_lookup(image[i + 1][2 * j][0], 1) +
                (uint16_t)colour_lookup(image[i][2 * j + 1][0], 1) +
                (uint16_t)colour_lookup(image[i + 1][2 * j + 1][0], 1)) / 4.0;
      const float hi2 =
        (float)((uint16_t)colour_lookup(image[i][2 * j][0], 2) +
                (uint16_t)colour_lookup(image[i + 1][2 * j][0], 2) +
                (uint16_t)colour_lookup(image[i][2 * j + 1][0], 2) +
                (uint16_t)colour_lookup(image[i + 1][2 * j + 1][0], 2)) / 4.0;
      float freq = 1500.0 + ((128.0 + (0.003906 * ((-37.945 * hi0) + (-74.494 * hi1) + (112.439 * hi2)))) * 3.1372549);
      set_dac_freq(freq);
      delayMicroseconds(ROBOT36_CSCAN_DELAY); // Ideal: 275
    }
  }

  // EOF
  set_dac_freq(1500);
  delay(300);
  set_dac_freq(1900);
  delay(100);
  set_dac_freq(1500);
  delay(100);
  set_dac_freq(1900);
  delay(100);
  set_dac_freq(1500);
  delay(100);

  // FSKID
  generate_fskid_char(0x20);
  generate_fskid_char(0x2A);
  generate_fskid_str(callsign + callsign_suffix);
  generate_fskid_char(0x01);
  dac_cw_generator_disable();
  delay(100);

  digitalWrite(PIN_RAD_PTT, HIGH);
  tune_dra818("144.3900");

}

// ---------------------------------------------------------------
// SSTV scheduling + transmit, called from RTTY_TX() right after each
// APRS packet. The old SSTV state machine is gone: pairing with APRS
// is structural now (a frame can only ever start immediately after a
// packet), so no handshake flags are needed.
// ---------------------------------------------------------------

static unsigned long last_sstv_time = 0;

// Decide whether an SSTV frame should follow the APRS packet that just
// finished. Mirrors the old SSTV_IDLE gating exactly.
bool sstv_due(unsigned long curr_time) {
  // Only allow SSTV after SSTV_inhibit_height_m height
  if (galtitude > SSTV_inhibit_height_m || paltitudeMSL > SSTV_inhibit_height_m) {
    inhibit_sstv = false;
  }
  // Or after it has been more than SSTV_inhibit_time_ms time
  if (curr_time > SSTV_inhibit_time_ms) {
    inhibit_sstv = false;
  }

  if (fixType == 0) use_gps = false;        // No Fix
  else if (fixType == 1) use_gps = false;   // Dead reckoning
  else if (fixType == 2) use_gps = true;    // 2D
  else if (fixType == 3) use_gps = true;    // 3D
  else if (fixType == 4) use_gps = true;    // GNSS + Dead reckoning
  else if (fixType == 5) use_gps = false;   // Time only

  // No matter what, if the GPS is not functioning, we don't SSTV to give
  // GPS a chance to lock on. [Comment out the next line for indoor bench
  // tests - RBF: restore before flight]
  //if (!use_gps) inhibit_sstv = true;

  if (!im_buf) return false;

  // Boot / forced frame bypasses the inhibits, same as the original
  // (sstv_run_now || ...) condition did.
  if (sstv_run_now) return true;

  if (inhibit_sstv) return false;


  if (fast_sstv) return true;

  // Normal schedule: fire on every multiple of SSTV_mod_m minutes
  // (SSTV_mod_m = 5 -> :00, :05, :10, :15 ... 12x/hour). Guard against
  // SSTV_mod_m == 0: that used to be reachable via a uint8_t truncation
  // of a float literal (0.5 -> 0) and would make this a mod-by-zero.
  // It's a fixed literal today, but the guard costs nothing and removes
  // the failure mode permanently.
  if (SSTV_mod_m == 0) return false;

  return (((tminute % SSTV_mod_m) == 0) &&
          ((curr_time - last_sstv_time) >= ((SSTV_mod_m * 60000L) - 60000L)));
}


void build_sstv_image() {
  char sbuf[128] = {0};

  im_ref = BEAR_IMAGES[bear_image_index].ref;
  im_cm  = BEAR_IMAGES[bear_image_index].cm;
  bear_image_index = (bear_image_index + 1) % BEAR_IMAGE_COUNT;

  memcpy(im_buf, im_ref, WIDTH * HEIGHT * COMPONENTS);
  im->setTextColor(0);
  im->setCursor(0, 0);
  im->setTextSize(4);
  im->print(callsign + callsign_suffix);

  im->setCursor(174, 207);
  im->setTextSize(4);
  im->print("BEAR14");

  // We start custom output at 150x35
  // That gives us 170x65 (without belly) or 170x135 of usable space
  // Font size 2: 15x4 chars | Font size 1: 34x8
  im->setCursor(149, 37);
  im->setTextSize(2);

  // Line 1: Date Time
  sprintf(sbuf, "%02d/%02d/%02d %02d:%02d", tday, tmonth, tyear % 100, thour, tminute, tsecond);
  im->print(sbuf);

  // Line 2: Lat
  im->setCursor(149, 54);
  sprintf(sbuf, "Lat %.4f", glatitude);
  im->print(sbuf);

  // Line 3: Lon
  im->setCursor(149, 71);
  sprintf(sbuf, "Lon %.4f", glongitude);
  im->print(sbuf);

  // Line 4: Alti Speed
  im->setCursor(149, 88);
  sprintf(sbuf, "%.0fm %.0fkm/h", galtitude, gspeed);
  im->print(sbuf);

  // Line 5: Temp Press
  im->setCursor(162, 105);
  sprintf(sbuf, "%d*C %uhPa", (int8_t)ambient_temp, baro_press);
  im->print(sbuf);

  imready = false;
}

// Radio handling + audio for one complete frame. Blocking (~36 s).
// Enters and leaves on 144.3900 with PTT released (HIGH = RX).
void sstv_transmit_frame() {

  robot_img(im_buf);

  last_sstv_time = millis();
  sstv_run_now = false;
}

void setup_sstv() {
  im = new GFXcanvas8(WIDTH, HEIGHT);
  if (!im) {
    Serial.println("ERR: Unable to instantiate GFXcanvas8");
  }
  
  im_buf = (uint8_t (*)[WIDTH][COMPONENTS])im->getBuffer();
  if (!im_buf) {
    Serial.println("ERR: Unable to get im_buf");
  }
  
  im->setTextColor(0);
  // Font is 5x8 per character
  im->setTextSize(1);
  im->setTextWrap(true);
}
