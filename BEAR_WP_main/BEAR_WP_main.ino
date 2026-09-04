// These headers define types used in function signatures elsewhere in the
// sketch (twai_message_t in CAN_BUS.ino's handle_can_frame(); JPEGDRAW in
// this file's drawMCUs()). Arduino auto-generates forward prototypes for
// every function in the sketch and inserts them near the top of this
// file - before this point - so each type has to already be visible
// here, even though the "real" #include for it sits further down (in
// CAN_BUS.ino, or later in this same file). Leaving the originals in
// place too is fine - header guards make the repeat harmless.
#include <string.h>
#include "JPEGDEC.h"

//TO DO:
//APRS PARAMETERS
String callsign = "9V1WP";
String callsign_suffix = "-11";
uint8_t callsign_ssid = 11;
String comment_suffix = "SSTV 145.670";
String boot_message = "BEAR14 Project";

//APRS
uint16_t msg_id = 0;
bool freefall=false;

//RBF - Set this to false before flight (for testing)
bool use_gps = true;

// RBF - Set this to false before flight
bool sstv_run_now = true;

// RBF - Set this to true before flight
bool inhibit_sstv = false;
//check SSTV.INO Line 221

// RBF - Set this to false before flight
bool fast_sstv = false;


//CUTTER CONFIG
const float CUT_ALTITUDE = 30000;  //input as  meters, Agreed standard unit)
//const float CUT_ALTITUDE = 0;  //input as  meters, Agreed standard unit)
bool cutterOn = false;                       //for use in telemetry only
const unsigned long CUT_DURATION = 15000;    //cut duration in ms

//timing variables
const unsigned long sensor_interval = 2000;  // read sensors every 100ms
const unsigned long setup_interval = 2000;
const unsigned long GNSS_interval = 1000;  //poll GNSS every 1 second


//SSTV variables
uint8_t SSTV_mod_m = 5;   
uint16_t SSTV_inhibit_height_m = 0;
uint32_t SSTV_inhibit_time_ms = 0L;
//uint32_t SSTV_inhibit_time_ms = 1200000L; // 20 minutes



// Enum to represent the states of our RTTY transmission
enum RTTYState {
  RTTY_IDLE,
  RTTY_START,
  RTTY_TRANSMITTING,
  RTTY_COOLDOWN
};

// rtty specific baro_temptiming variables
RTTYState rttyState = RTTY_IDLE;
unsigned long rttyStateStartTime = 0;
const unsigned long RTTY_IDLE_TIME = 30000;      // 20 seconds between transmissions
const unsigned long RTTY_START_TIME = 1000;      // 250 ms idle signal
const unsigned long RTTY_COOLDOWN_TIME = 250;    // 250 second cooldown




// Global variables for UART BRIDGE

volatile bool newTelemetryReady = false;
volatile float BV = 0.0;
volatile float Current = 0.0;
volatile float BatTemp = 0.0;
volatile uint8_t heaterOn = 0;


// Received from HEAD over the I2C link (see HEAD_LINK.ino) 


// Camera
bool imready = false;

//RADIO PINS

// GPIO where the DS18B20 is connected to
//#include <OneWire.h>
//#include <DallasTemperature.h>
// Setup a oneWire instance to communicate with any OneWire devices
//#define oneWireBus 17
// Used to be GPIO35 but it's INPUT only so switched to GPIO17 or the seventh pin on the left side of the esp32
//OneWire oneWire(oneWireBus);
// Pass our oneWire reference to Dallas Temperature sensor
//DallasTemperature onesense(&oneWire);

//MCP9600 thermocouple
//#include "Adafruit_MCP9600.h"
//#define MCP9600_ADDR (0x60)
//Adafruit_MCP9600 mcp;
//const int thermocouple_wakeup = 500;  //give 250ms to wakeup
//float cold_junc = 0.0f;
//uint16_t thermocouple_adc = 0;

//MS5611 baro
#include "MS5611.h"
MS5611 MS5611(0x77);
//int16_t baro_temp = 30;  //needs to be signed due to -ve temps


//GPS
const char assistNowServer[] = "https://online-live1.services.u-blox.com";
//const char assistNowServer[] = "https://online-live2.services.u-blox.com"; // Alternate server

const char getQuery[] = "GetOnlineData.ashx?";
const char tokenPrefix[] = "token=";
const char tokenSuffix[] = ";";
const char getGNSS[] = "gnss=gps,glo,qzss,bds,gal;";  // GNSS can be: gps,qzss,glo,bds,gal
const char getDataType[] = "datatype=eph,alm,aux;";   // Data type can be: eph,alm,aux,pos

#ifdef USE_SERVER_ASSISTANCE
const char useLatitude[] = "lat=1.3521;";     // Use an approximate latitude of 55 degrees north. Replace this with your latitude.
const char useLongitude[] = "lon=103.8198;";  // Use an approximate longitude of 1 degree west. Replace this with your longitude.
const char useAlt[] = "alt=100;";             // Use an approximate latitude of 100m above WGS84. Replace this with your altitude.
const char usePosAcc[] = "pacc=60000;";       // Use a position accuracy of 60000m (60km)
#endif

#include <SparkFun_u-blox_GNSS_Arduino_Library.h>  //http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS myGNSS;

#include "time.h"

// The Network Time Protocol Servers, ntpServer is main, the rest are back ups
const char* ntpServer = "time.nist.gov";
const char* ntpServer_01 = "1.pool.ntp.org";
const char* ntpServer_02 = "0.pool.ntp.org";

// GPS
// Fix Type
// 0: No fix
// 1: Dead reckoning
// 2: 2D
// 3: 3D
// 4: GNSS + Dead reckoning
// 5: Time only

byte fixType = 0;
float glatitude = 1.3521;   //CHANGE BEFORE FLIGHT
float glongitude =  103.8198;  //CHANGE BEFORE FLIGHT
float galtitude = 0;
float paltitudeMSL=0;
float gspeed = 0;
uint16_t gheading = 0;

//RTC stuff [RBF]
uint16_t tyear=2026;
uint8_t tmonth=8;
uint8_t tday=28;
uint16_t thour = 6;
uint16_t tminute = 6;
uint16_t tsecond = 6;
uint16_t tms = 6;
unsigned long long tsync = 0;


//CAM SWITCH PWM SETTINGS(not used in BEAR14)
// Hardware PWM Settings
//const int ledcChannel = 0; //APRS
//const int ledcChannel2= 2; //vtx switcher 
//const int ledcFreq = 50;      // 50Hz (20ms period)
//const int ledcRes = 13;       // 13-bit resolution (0-8191)


// PACKET FORMAT AND VARIABLE DECLARATIONS
uint8_t frame_counter = 0;
float ambient_temp = 0.0f; //barometer temp
float external_temp=0.0f; //type K temp
uint16_t baro_press = 0;



//////////////////////////////////////////////////////
//// Pinmap
//////////////////////////////////////////////////////

#define PIN_DTR 0
// Define VSPI pins
#define VSPI_SCK 18
#define VSPI_MISO 19
#define VSPI_MOSI 23
#define VSPI_SS 27

//I2C sensors setup
#define I2C_SDA 21
#define I2C_SCL 22


//THIS IS THE SDA/SCL ON THE 1X06 JST HEADER
//HEAD LINK (I2C slave here - see i2c_HEAD.ino; HEAD is master, writing
//on its own Wire bus).
#define I2C2_SDA 32 //I2C SDA
#define I2C2_SCL 33 //I2C SCL

//CAN BUS PIN MAPPING (TWAI controller -> external transceiver, e.g. SN65HVD230)
//TODO(E): not wired anywhere else in this project - confirm against actual
//transceiver wiring. 26/34 are just the free, non-strapping pins available here.
#define CAN_TX 4
#define CAN_RX 5


//camera
#define PIN_CAM_TX 16
#define PIN_CAM_RX 17

//Camera Switch pins
//#define PIN_CAM_SWITCH 26

//DRA818V PIN MAPPING
#define PIN_RAD_PTT 2
#define PIN_RAD_EN 13
#define PIN_RAD_PW 12
#define PIN_RAD_SQL 39 //sensor_vn
#define PIN_RAD_TX 15 //HARDWARE FLIPPED ALREADY
#define PIN_RAD_RX 14 //HARDWARE FLIPPED ALREADY
#define PIN_TX_AUD 25
#define PIN_RX_AUD 36 //sensor_VP

// include the libraries
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include <CAN.h>


//temperature sensr
#include "Adafruit_MAX31855.h"
// Initialize with Software SPI to force the specific VSPI pins
//Adafruit_MAX31855 maxthermo = Adafruit_MAX31855(VSPI_SS, VSPI_MOSI, VSPI_MISO, VSPI_SCK);
Adafruit_MAX31855 thermocouple( VSPI_SCK, VSPI_SS, VSPI_MISO);

#include <WiFi.h>
#include <HTTPClient.h>
#include "secrets.h"

//APRS
#include <APRSLite.h>


////////////////////////////////////////////////////////
//// DRA818
//////////////////////////////////////////////////////
#ifdef HW_ESP32S3
#include "hal/ledc_types.h"
#include "soc/ledc_periph.h"
#include "soc/ledc_struct.h"
#include "hal/gpio_hal.h"
#include "esp_rom_gpio.h"
ledc_dev_t *ledc = &LEDC;
#else
#include "soc/rtc_io_reg.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "soc/rtc.h"
#include <driver/dac.h>
#include <hal/dac_hal.h>
#include <hal/dac_ll.h>
#endif
#include <SoftwareSerial.h>
SoftwareSerial radioCtrl(PIN_RAD_RX, PIN_RAD_TX);

//////////////////////////////////////////////////////
//// WIFI
//////////////////////////////////////////////////////
#include <WiFi.h>
#include <WiFiAP.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <DynamicArduinoOTA.h>
#include <UDPStream.h>
#include <esp_wifi.h>

UDPStream udpstream;

//////////////////////////////////////////////////////
//// SSTV
//////////////////////////////////////////////////////
#include "Adafruit_GFX.h"

#include "Adafruit_GFX.h"

#include "BEAR_images.h"
#define WIDTH       BEAR_IMAGE_WIDTH
#define HEIGHT      BEAR_IMAGE_HEIGHT
#define COMPONENTS  BEAR_IMAGE_COMPONENTS

// The outer pointer is not const-qualified (only the pointee is), so
// SSTV.ino can reassign these to cycle through BEAR_IMAGES[] each time
// it falls back to a baked-in frame.
const uint8_t (*im_ref)[WIDTH][COMPONENTS] = BEAR_IMAGES[0].ref;
const uint8_t (*im_cm)[4] = BEAR_IMAGES[0].cm;
uint8_t bear_image_index = 1;  // next index to use (0 was assigned above)

#define HALFWIDTH   (WIDTH/2)
#define HALFHEIGHT  (HEIGHT/2)

uint8_t (*im_buf)[WIDTH][COMPONENTS] = 0;
GFXcanvas8 *im;


//////////////////////////////////////////////////////
//// Camera
//////////////////////////////////////////////////////
#include <Adafruit_VC0706.h>
#include "JPEGDEC.h"

 
Adafruit_VC0706 cam = Adafruit_VC0706(&Serial1);
JPEGDEC jpeg;

#define MAX_ALLOWED_JPG_SIZE (32000)
uint8_t jpg_img[MAX_ALLOWED_JPG_SIZE];
uint16_t jpg_sz = 0;

int drawMCUs(JPEGDRAW *pDraw) {
  if (!im_buf) return 0;

  int x = pDraw->x;
  int y = pDraw->y;
  int w = pDraw->iWidth;
  int h = pDraw->iHeight;

  for (int16_t i = 0; i < w; i++) {
    for (int16_t j = 0; j < h; j++) {
      uint16_t pval = pDraw->pPixels[i + j * w];
      im_buf[y + j][x + i][0] = idx_nearest_colour((pval & 0xF800) >> 8, (pval & 0x07E0) >> 3, (pval & 0x001F) << 3);;
    }
  }
  return 1;
}

//////////////////////////////////////////////////////
/// i2c bus to head
//////////////////////////////////////////////////////

// Dedicated bus to HEAD - separate from Wire (TAIL's own baro/GPS), see
// I2C2_SDA/I2C2_SCL above and HEAD_LINK.ino.
TwoWire I2C2_Bus = TwoWire(1);




