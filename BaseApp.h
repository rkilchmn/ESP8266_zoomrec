#ifndef BASEAPP_H
#define BASEAPP_H

/**
 * How to
 * 1) Erase flash: esptool.py --chip ESP8266 -p /dev/ttyUSB0  erase_flash
 * 2) Deep sleep for Wemos D1 Mini: connect D0 with RST using schottky diode (e.g. BAT43, the cathode (ring) towards gpio16/D0 )
 *    to cleanly pull rst low without side effects when gpio16 is high. Workaround is just connect via wire
 */

#include "features.h"

#include <Arduino.h>
#include <stdarg.h>
#include <ESP8266WiFi.h> // git clone https://github.com/tzapu/WiFiManager.git
#include <Ticker.h>

#include "Config.h"

#ifdef TELNET
#include "TelnetStreamBuffered.h"
#endif

#ifdef WIFI_PORTAL
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#endif

#ifdef ARDUINO_OTA
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#endif

#ifdef HTTP_OTA
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#endif

#ifdef JSON_CONFIG_OTA
#include <FS.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h> // git clone https://github.com/bblanchon/ArduinoJson.git
#endif

#ifdef USE_NTP
#include <TZ.h>
#include <coredecls.h> // optional callback to check on server
#endif

#ifdef DEEP_SLEEP_SECONDS
#include <RTCVars.h>     // git clone https://github.com/highno/rtcvars.git
#endif


class BaseApp
{
public:
  BaseApp();
  ~BaseApp();

  void setup();
  void loop();

protected:
  String FIRMWARE_VERSION = String(__FILE__) + "-" + String(__DATE__) + "-" + String(__TIME__);
  const uint8 WATCHDOG_SETUP_SECONDS = 30; // Setup should complete well within this time limit
  const uint8 WATCHDOG_LOOP_SECONDS = 20;  // Loop should complete well within this time limit
  Ticker watchdog;

  Console console;
  Config config;

  virtual void AppSetup();     // ovveride this if reqired
  virtual void AppLoop();      // ovveride this if reqired
  virtual void AppIntervall(); // ovveride this if reqired
  virtual void AppFirmwareVersion(); // implement in the app class

  String getResetReasonString(uint8_t reason);
  void timeoutCallback();
  bool connectWiFi();

#ifdef WIFI_PORTAL
  // const uint8 WIFI_PORTAL_TRIGGER_PIN = 4; // A low input on this pin will trigger the Wifi Manager Console at boot. Comment out to disable.
  void configModeCallback(WiFiManager *myWiFiManager);
  WiFiManager wifiManager;
#else
  const char *WIFI_SSID = "SSID";
  const char *WIFI_PASSWORD = "password";
#endif

#ifdef TELNET
  const int TELNET_DEFAULT_PORT = 23;
  TelnetStreamBuffered *pBufferedTelnetStream = nullptr;
#endif

#ifdef ARDUINO_OTA
  const int ARDUINO_OTA_PORT = 8266;
  const char *ARDUINO_OTA_PASSWD = "1csqBgpHchquXkzQlgl4";
  void setupArduinoOta();
#endif

#ifdef HTTP_OTA
  const char *HTTP_OTA_URL = "http://192.168.0.1:8080/firmware";
  const char *HTTP_OTA_USERNAME = "admin";
  const char *HTTP_OTA_PASSWORD = "myadmin";
  void setupHTTPOTA();
  boolean performHttpOtaUpdate();
#endif

  const int SERIAL_DEFAULT_BAUD = 74880;     // native baud rate to see boot message
  const int FAST_CONNECTION_TIMEOUT = 10000; // timeout for initial connection atempt

  const char *SSID = (String("ESP") + String(ESP.getChipId())).c_str();

#ifdef TIMER_INTERVAL_MILLIS
  unsigned long timer_interval = 0;
#endif

#ifdef DEEP_SLEEP_SECONDS
  bool preventDeepSleep = false; // allow the app to prevent vom going to deep sleep in certain conditions
  unsigned long timer_coldboot = 0; // initialize time to keep track of timer for cold boot startup
  RTCVars deepSleepState;  // storage of state variables in reset/deepSleep-safe RTC memory - define variables in App contructor
  virtual void AppDeepSleepStateInit(); // ovveride this if reqired
#endif

#ifdef LED_STATUS_FLASH
  const uint8 STATUS_LED = LED_BUILTIN; // Built-in blue LED change if required e.g. pin 2
  uint8 LED_ON = LOW;                   // Define the value to turn the LED on
  uint8 LED_OFF = HIGH;                 // Define the value to turn the LED off
  Ticker flasher;
  void flash();
#endif

#ifdef USE_NTP
  // from https://werner.rothschopf.net/202011_arduino_esp8266_ntp_en.htm
  const char *NTP_SERVER = "pool.ntp.org";
  // Timezone definition https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  boolean ntp_set = false; // has ntp time been set
  void setupNtp();
  void time_is_set(boolean from_sntp /* <= this optional parameter can be used with ESP8266 Core 3.0.0*/);
  uint32_t sntp_startup_delay_MS_rfc_not_less_than_60000();
  uint32_t sntp_update_delay_MS_rfc_not_less_than_15000();
#endif

  // Add your class members and methods here based on the provided code.
};

#endif // BASEAPP_H
