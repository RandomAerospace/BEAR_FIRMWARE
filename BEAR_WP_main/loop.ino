void loop() {
  int tx_counter = 0;


  Cutter();
  if (newTelemetryReady) {
    // Safe to print here!
    Serial.printf("BV=%.2fV Current=%.3fA BatTemp=%.1fC heaterOn=%d\n", BV, Current, BatTemp, heaterOn);
    
    // Lower the flag so we don't print again until the next packet
    newTelemetryReady = false; 
  }

  // ... the rest of your normal loop code ...

  // Non-blocking, run every iteration regardless of RTTY state - see
  // setup_canbus() for why the RX queue is widened to match.

  //checks the battery temp every 5s, changes state every 5s, also checks if RTTY is going to be transmitted.
  //this is why it is placed at the front of the loop because i don't want GPIO to be high before RTTY Tx.
  //also the heater eats too much current with the energizer lithiums, it will cause the esp32 to bootloop

  //only during IDLE and cutterOn FALSE will sensors update
  if (rttyState == RTTY_IDLE) {
    updateSensors();
  }
  // APRS + (when due) SSTV, one merged state machine.
  // SSTV frames are sent by RTTY_TX itself, immediately after the
  // APRS packet - see RTTY_TRANSMITTING in dra818.ino.
  RTTY_TX();
}



//THIS FUNCTION UPDATES SENSORS ACCORDING TO RTTY_STATE
void updateSensors() {
  unsigned long currentMillis = millis();
  static unsigned long previousMillis = 0;
  static unsigned long previousGNSSMillis = 0;
  static unsigned long previousBATTEMPMillis = 0;

  // Break out early if we're not in RTTY_IDLE state
  if (rttyState != RTTY_IDLE) {
    return;
  }
  if (currentMillis - previousGNSSMillis >= GNSS_interval) {  //this happens every 1000ms
    read_gnss();

    previousGNSSMillis = currentMillis;
  }

  if (currentMillis - previousMillis >= sensor_interval) {  //this happens every 250ms
    read_baro();
    task_temp();


    previousMillis = currentMillis;
  }
  /*
  // Add debugging
  static unsigned long last_debug_time = 0;
  if (currentMillis - last_debug_time >= 5000) {  // Print every 5 seconds
    Serial.print(F("[DEBUG] RTTY State: "));
    Serial.print(rttyState);
    
    Serial.print(F(" Heater Pin: "));
    Serial.print(digitalRead(PIN_HEATER));
    Serial.print(F(" Temp: "));
    Serial.println(ambient_temp);
    last_debug_time = currentMillis;
  }
  */
}
