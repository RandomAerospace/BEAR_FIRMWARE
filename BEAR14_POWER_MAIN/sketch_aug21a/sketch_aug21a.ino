#define PIN_DTR 0

//CAN, note:no TX/RX swapping
#define CAN_RX 4
#define CAN_TX 5

#define GPIO_RST 18
#define GPIO_INT 19

//io extension
#define I2C_SDA 21
#define I2C_SCL 22

//current sense and i2c bridge
#define I2C2_SDA 32
#define I2C2_SCL 33

#define onewire_battery 25   //oneWire1 silkscreen
#define onewire_external 23  //onWire2 silkscreen

// include the libraries
#include "time.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include <Adafruit_INA228.h>



//Onewire
#include <OneWire.h>
#include <DallasTemperature.h>



//OneWire oneWireBattery(onewire_battery);
//for flight 14, we are using external bus due to issues
OneWire oneWireExternal(onewire_external);
//DallasTemperature sensorsBattery(&oneWireBattery);
DallasTemperature sensorsExternal(&oneWireExternal);


//IO EXTENDER
#include "TCA6408A.h"
TCA6408A tca(0x21, &Wire);
// Mirrors the IO extender's output register (bit n = Pn, 1 = rail ON) -
// see IO_EXTENDER.ino for the BUS_* bit positions.
uint8_t busState = 0x38;


//note: the original zanshin libary is modified by me so that the second i2c bus can be used!
#include <INA.h>  // Zanshin INA Library
//CURRENT SENSE
const uint32_t SHUNT_MICRO_OHM{ 6800 };  ///< Shunt resistance in Micro-Ohm, e.g. 100000 is 0.1 Ohm
const uint16_t MAXIMUM_AMPS{ 12 };       ///< Max expected amps, clamped from 1A to a max of 1022A
uint8_t devicesFound{ 0 };               ///< Number of INAs found
INA_Class INA;                           ///< INA class instantiation to use EEPROM

static char sprintfBuffer[100];                                       // Buffer to format output
static char busChar[8], shuntChar[10], busMAChar[10], busMWChar[10];  // Output buffers


// Create the second I2C instance (using hardware I2C peripheral 1)
TwoWire I2C2_Bus = TwoWire(1);

// Global variables for UART BRIDGE
float BV = 0.0f;
float Current = 0;
bool heaterOn = false;
float BatTemp = 0.0;
float extTemp = 0.0;

void setup() {
  // put your setup code here, to run once:
  //pinMode(PIN_DTR, INPUT_PULLUP); //pull up dtr
  Serial.begin(115200);

  //initialise i2c
  Serial.print("Initialising I2c");
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);
  Serial.print("Initialising I2c2");
  I2C2_Bus.begin(I2C2_SDA, I2C2_SCL);
  I2C2_Bus.setClock(100000);
  // I2C2_Bus belongs entirely to the INA228 current sensor - whatever
  // initializes it for that (setup_currentsense(), below) is untouched.
  // setup_tail_link() no longer calls begin() on anything here; it reuses
  // this same Wire bus, already master to the TCA6408A.
  setup_IOEXTENDER();
  Serial.print("IO EXTENDER");
  nominal_flight();  //sets states for the GPIO extender.
  Serial.print("NOMINAL FLIGHT SET");
  setup_currentsense();
  Serial.print("CURRENT SENSE SET");
  setup_onewire();
  Serial.print("ONEWIRE SET");
  setup_tail_link();
  Serial.print("TAIL LINK SET");


  setup_webserver();
}

void loop() {
  // put your main code here, to run repeatedly:
  read_currentsense();
  read_temps();
  task_heater();
  send_head_telemetry();
  handle_webserver();  // serve the power-control page until flight state kills WiFi
  delay(1000);

 
}
