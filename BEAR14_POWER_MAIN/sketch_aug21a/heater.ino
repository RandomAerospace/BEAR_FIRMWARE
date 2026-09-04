void setup_onewire(void) {
  Serial.println("Dallas Temperature IC Control Library Demo");
  //sensorsBattery.begin();
  sensorsExternal.begin();
}

void read_temps(void) {
  Serial.print("Requesting temperatures...");
  //sensorsBattery.requestTemperatures();
  sensorsExternal.requestTemperatures();
  Serial.println("DONE");

  //float temp_battery = sensorsBattery.getTempCByIndex(0);
  float temp_battery = sensorsExternal.getTempCByIndex(0);

  if (temp_battery != DEVICE_DISCONNECTED_C) {
    Serial.print("Battery_temp:  ");
    Serial.println(temp_battery);
    BatTemp = temp_battery;  //update global variable
  } else {
    Serial.println("Error: Could not read battery temperature data");
  }
  /*
  if (temp_external != DEVICE_DISCONNECTED_C) {
    Serial.print("External_temp: ");
    Serial.println(temp_external);
    extTemp = temp_external;  //update global variable
  } else {
    Serial.println("Error: Could not read external temperature data");
  }
  */
}

void task_heater(void) {
  //takes global variable temp_battery and decides if it needs to turn on heaters
  //Change flag->actually change the GPIO
  //we don't use else statements so that we can gate conditions to if cases ONLY
  // 1. Run Heater Logic
  // 1. Hardware Failsafe: Catch the -127 disconnected error immediately
  if (BatTemp <= -100.0) {
    heaterOn = false;
  }
  // 2. Normal Control Logic: Lower threshold turns heater ON
  else if (BatTemp <= 15.0) {
    heaterOn = true;
  }
  // 3. Normal Control Logic: Upper threshold turns heater OFF
  else if (BatTemp >= 20.0) {
    heaterOn = false;
  }

  //actually changing the GPIO state
  if (heaterOn == true) {
    heater_on();
  } else {
    heater_off();  //no point turning on
  }
}
