void setup() {
  unsigned long currentMillis = millis();
  static unsigned long previousMillis = 0;
  // Bug fix, pullup DTR so we can boot properly if we reset
  pinMode(PIN_DTR, INPUT_PULLUP);
  Serial.begin(115200);
  // Small delay to avoid reset during SPI initialization


  //initialise i2c
  Serial.print("Initialising I2c");
  Wire.begin(I2C_SDA,I2C_SCL);
  Wire.setClock(100000); 
  //i2c2 is initalised in head.ino

  Serial.println("HEAD link");
  setup_head_link();
  Assistnow_setup(); 
  GNSS_setup();
  
  // Will transmit boot message, ensure dra818 is ready
  //setup DRA818 first before APRS.
  Serial.println("DRA818");
  setup_dra818();
  Serial.println("APRS");
  setup_aprs();

  

  baro_setup();

  
  //setup_camera();
  setup_sstv();
  Cutter_setup();
  Serial.print("All setup");


  
}

