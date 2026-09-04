//GPIO extender
//P0->GPIO2_EN [MSB]
//P1->GPIO1_EN
//P2->HEATER_EN (12V BUS 1)
//P3->5VDC_BUS3_EN
//P4->5VDC_BUS1_EN
//P5->5VDC_BUS2_EN
//P6->VTX_EN    (12V BUS 2)
//P7-> NC

//P7 is lsb

// NOTE: checked against the working nominal_flight()/heater_off() values below -
// tca.digitalWrite8() actually maps Pn -> bit n (P0 = bit0/LSB ... P7 = bit7/MSB),
// the reverse of "[MSB]" / "P7 is lsb" above. Bit = 1 means the rail is ON (active high).
#define BUS_GPIO2   0   // P0 - GPIO2_EN
#define BUS_GPIO1   1   // P1 - GPIO1_EN
#define BUS_HEATER  2   // P2 - HEATER_EN (12V BUS 1)
#define BUS_5V3     3   // P3 - 5VDC_BUS3_EN
#define BUS_5V1     4   // P4 - 5VDC_BUS1_EN
#define BUS_5V2     5   // P5 - 5VDC_BUS2_EN
#define BUS_VTX     6   // P6 - VTX_EN (12V BUS 2)
// P7 - NC, not controllable

void setup_IOEXTENDER() {
  pinMode(GPIO_RST, OUTPUT);
  digitalWrite(GPIO_RST, LOW);      // hold in reset
  delayMicroseconds(10);            // >> tW (40 ns)
  digitalWrite(GPIO_RST, HIGH);     // release
  delayMicroseconds(10);            // >> tRESET (600 ns)
  tca.digitalWrite8(0xFF);          // output register first — all rails OFF
  //  Set ALL PINS TO BE OUTPUTS
  //  0X 00 00000000
  tca.setPinMode8(0x00);
  
  //  Initialize outputs to OFF
  //  0XFF  00000000 (state zero to OFF)
  tca.digitalWrite8(0xFF);
  Serial.println("IO_Extender_configuration done");
}

// Flip a single rail (see BUS_* above) without disturbing any of the others -
// used by heater_on()/heater_off() and the web control panel so automatic
// heater control and manual web toggles don't stomp on each other.
void set_bus(uint8_t bit, bool on) {
  if (on) busState |= (1 << bit);
  else    busState &= ~(1 << bit);
  tca.digitalWrite8(busState);
}

//probably would want a state machine triggered by diff modes
void nominal_flight(){

  // 5V_BUS1 (P4), 5V_BUS2 (P5), 5V_BUS3 (P3) ON; heater and VTX OFF
  // 0x38 = 00111000, fets are active high

  busState = 0x38;
  tca.digitalWrite8(busState);
}

void heater_on(){
  set_bus(BUS_HEATER, true);
}

void heater_off(){
  set_bus(BUS_HEATER, false);
}
