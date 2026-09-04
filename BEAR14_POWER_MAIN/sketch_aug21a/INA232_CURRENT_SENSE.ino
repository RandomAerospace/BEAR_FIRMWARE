
void setup_currentsense() {
  Serial.print("\n\nDisplay INA Readings V1.0.8\n");
  Serial.print(" - Searching & Initializing INA devices\n");
  /************************************************************************************************
  ** The INA.begin call initializes the device(s) found with an expected ±1 Amps maximum current **
  ** and for a 0.1Ohm resistor, and since no specific device is given as the 3rd parameter all   **
  ** devices are initially set to these values.                                                  **
  ************************************************************************************************/
  devicesFound = INA.begin(MAXIMUM_AMPS, SHUNT_MICRO_OHM);  // Expected max Amp & shunt resistance
  while (devicesFound == 0) {
    Serial.println(F("No INA device found, retrying in 10 seconds..."));
    delay(10000);                                             // Wait 10 seconds before retrying
    devicesFound = INA.begin(MAXIMUM_AMPS, SHUNT_MICRO_OHM);  // Expected max Amp & shunt resistance
  }                                                           // while no devices detected
  Serial.print(F(" - Detected "));
  Serial.print(devicesFound);
  Serial.println(F(" INA devices on the I2C bus"));
  INA.setBusConversion(8500);             // Maximum conversion time 8.244ms
  INA.setShuntConversion(8500);           // Maximum conversion time 8.244ms
  INA.setAveraging(128);                  // Average each reading n-times
  INA.setMode(INA_MODE_CONTINUOUS_BOTH);  // Bus/shunt measured continuously
  //INA.alertOnBusOverVoltage(true, 5000);  // Trigger alert if over 5V on bus, A0 is disabled
}


void read_currentsense() {
/*!
   * @brief    Arduino method for the main program loop
   * @details  This is the main program for the Arduino IDE, it is an infinite loop and keeps on
   * repeating. In order to format the output use is made of the "sprintf()" function, but in the
   * Arduino implementation it has no support for floating point output, so the "dtostrf()" function
   * is used to convert the floating point numbers into formatted strings.
   * @return   void
   */
  static uint16_t loopCounter = 0;     // Count the number of iterations
  static char     sprintfBuffer[100];  // Buffer to format output
  static char     busChar[8], shuntChar[10], busMAChar[10], busMWChar[10];  // Output buffers

  Serial.print(F("Nr Adr Type   Bus      Shunt       Bus         Bus\n"));
  Serial.print(F("== === ====== ======== =========== =========== ===========\n"));
  for (uint8_t i = 0; i < devicesFound; i++)  // Loop through all devices
  {
    float busVolts      = (INA.getBusMilliVolts(i) * 1.28) / 1000.0;  // correct for INA232 LSB vs library's INA230 assumption;   // mV -> V
    float shuntVolts    = INA.getShuntMicroVolts(i) / 1000.0;   // uV -> mV
    float busMilliAmps  = INA.getBusMicroAmps(i)    / 1000000.0;   // uA -> mA
    float busMilliWatts = INA.getBusMicroWatts(i)   / 1000.0;   // uW -> mW

    dtostrf(busVolts, 7, 2, busChar);          // Convert floating point to char
    dtostrf(shuntVolts, 9, 2, shuntChar);      // Convert floating point to char
    dtostrf(busMilliAmps, 9, 2, busMAChar);    // Convert floating point to char
    dtostrf(busMilliWatts, 9, 2, busMWChar);   // Convert floating point to char
    sprintf(sprintfBuffer, "%2d %3d %s %sV %smV %smA %smW\n", i + 1, INA.getDeviceAddress(i),
            INA.getDeviceName(i), busChar, shuntChar, busMAChar, busMWChar);
    Serial.print(sprintfBuffer);

    if (i == 0) {                        // device 0 = the battery rail -> global telemetry
      BV      = busVolts;                // Volts
      Current = busMilliAmps;   
    }
  }  // for-next each INA device loop
}