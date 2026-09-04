
inline uint32_t col_diff(int32_t r1, int32_t g1, int32_t b1, int32_t r2, int32_t g2, int32_t b2) {
  r1 -= r2;
  r1 *= r1;
  g1 -= g2;
  g1 *= g1;
  b1 -= b2;
  b1 *= b1;
  return r1 + g1 + b1;
}

inline uint8_t idx_nearest_colour(uint8_t r, uint8_t g, uint8_t b) {
  uint8_t ret = 0;
  uint32_t best_mag = 0xFFFFFFFFUL;
  for (uint16_t i = 0; i < 256; i++) {
    uint32_t mag = col_diff(r, g, b, im_cm[i][2], im_cm[i][1], im_cm[i][0]);
    if (mag < best_mag) {
      ret = i;
      best_mag = mag;
    }
  }
  return ret;
}

bool decode_jpg() {
  if (!jpg_img) imready = false;
  if (!jpg_sz) imready = false;
  if (!imready) return imready;

  if (jpeg.openRAM(jpg_img, jpg_sz, drawMCUs)) {
    Serial.println("Valid JPG");
    Serial.printf("Image size: %d x %d, orientation: %d, bpp: %d\r\n", jpeg.getWidth(), jpeg.getHeight(), jpeg.getOrientation(), jpeg.getBpp());
    jpeg.setPixelType(RGB565_LITTLE_ENDIAN);
    if (jpeg.decode(0, 0, 0)) {
      Serial.println("JPG decoded");
      imready = true;
    } else {
      Serial.println("JPG decode error");
      imready = false;
    }
    jpeg.close();
  } else {
    Serial.println("Invalid JPG");
    imready = false;
  }
  return imready;
}

bool take_photo() {
  imready = false;
  if (!jpg_img) return imready;

  uint8_t tries = 3;
  while (tries--) {
    Serial.print(tries);
    Serial.println(" tries left");

    if (!cam.takePicture()) {
      Serial.println("Failed to snap!");
      cam.resumeVideo();
      continue;
    }

    // Get the size of the image (frame) taken
    jpg_sz = cam.frameLength();
    Serial.print(jpg_sz, DEC);
    Serial.println(" byte image");

    if (!jpg_sz) {
      Serial.println("Img size 0!");
      cam.resumeVideo();
      continue;
    }

    // Since we use pre-allocated memory, the maximum size is fixed
    if (jpg_sz > MAX_ALLOWED_JPG_SIZE) {
      Serial.println("Img size too big!");
      cam.resumeVideo();
      continue;
    }

    uint16_t remaining = jpg_sz;
    uint8_t *jpg_img_p = jpg_img;
    while (remaining) {
      uint8_t *buf;
      uint8_t bytesToRead = min((uint16_t)64, remaining);
      buf = cam.readPicture(bytesToRead);
      // buf can be null if there was an error reading the picture
      if (!buf) {
        cam.resumeVideo();
        break;
      }
      memcpy(jpg_img_p, buf, bytesToRead);
      jpg_img_p += bytesToRead;
      remaining -= bytesToRead;
    }

    if (!remaining) {
      Serial.println("Img read!");
      imready = true;
      cam.resumeVideo();
      break;
    }

    cam.resumeVideo();
  }
  return imready;
}
// SSTV on BEAR14 transmits the static onboard images from BEAR_IMAGES[]
// only (see build_sstv_image() in SSTV.ino, which always overwrites
// im_buf from that array) - nothing in the sketch ever decodes a live
// camera frame into im_buf, so a photo captured here has no consumer.
// task_camera() used to mirror the SSTV modm schedule (offset 1 minute
// early) to have a fresh photo ready in time, but since that photo was
// never actually used, it was just burning camera UART time and battery
// in RTTY_IDLE for no effect. Left as a no-op stub, gated off, so the
// capture path (take_photo() etc.) stays available to wire back in if
// live-camera SSTV is ever revisited - see git history for the
// SSTV-schedule-mirroring trigger condition that used to live here.

void task_camera() {
  unsigned long curr_time = millis();
  static unsigned long last_time = 0;

  // This condition mirrors that of the SSTV module but modified to run 1 minute earlier
  if (sstv_run_now || (!inhibit_sstv && fast_sstv) || (!inhibit_sstv && ((((tminute + 1) % SSTV_mod_m) == 0) && (curr_time - last_time) >= ((SSTV_mod_m * 60000L) - 60000L)))) {
    take_photo();
    last_time = curr_time;
  }
}

void setup_camera() {
  //cam init sequence:
  //1.Power on
  //2.Delay 2.5s
  //3.Set image resolution
  //4.Set image compressibility
  //5.Camera reset
  Serial1.begin(115200, SERIAL_8N1, PIN_CAM_RX, PIN_CAM_TX);
  
  delay(2700);
  cam.reset();


 
  
  Serial1.begin(38400, SERIAL_8N1, PIN_CAM_RX, PIN_CAM_TX);
  cam.setImageSize(VC0706_320x240);
  // You can read the size back from the camera (optional, but maybe useful?)
  uint8_t imgsize = cam.getImageSize();
  Serial.print("Image size: ");
  if (imgsize == VC0706_640x480) Serial.println("640x480");
  if (imgsize == VC0706_320x240) Serial.println("320x240");
  if (imgsize == VC0706_160x120) Serial.println("160x120");

  // Switch to 115200
  cam.setBaud115200();
  
  Serial1.begin(115200, SERIAL_8N1, PIN_CAM_RX, PIN_CAM_TX);
    if (cam.reset()) {
    Serial.println("Camera Found:");
  } else {
    Serial.println("No camera found?");
    return;
  }

  // Print out the camera version information (optional)
  char *reply = cam.getVersion();
  if (reply == 0) {
    Serial.print("Failed to get version");
  } else {
    Serial.println("-----------------");
    Serial.print(reply);
    Serial.println("-----------------");
  }
}