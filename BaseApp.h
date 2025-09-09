#ifndef BASEAPP_H
#define BASEAPP_H

/**
 * How to
 * 1) Erase flash: esptool.py --chip ESP8266 -p /dev/ttyUSB0  erase_flash
 * 2) Deep sleep for Wemos D1 Mini: connect D0 with RST using schottky diode (e.g. BAT43, the cathode (ring) towards gpio16/D0 )
 *    to cleanly pull rst low without side effects when gpio16 is high. Workaround is just connect via wire
 */

#include "features.h"
#include "JSONAPIClient.h"

#include <Arduino.h>
#include <stdarg.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>

#include "Config.h"
#include <WiFiClientSecure.h>
#include <memory>

// Forward declaration to avoid including BearSSL headers here
namespace BearSSL { class PublicKey; }

#ifdef USE_MDNS
#include <ESP8266mDNS.h>
#endif

#ifdef CONSOLE_TELNET
#include "TelnetStreamBuffered.h"
#endif

#ifdef CONSOLE_HTTP
#include "HttpStreamBuffered.h"
#endif

#ifdef HTTP_CONFIG
#include "JSONAPIClient.h"
#endif

#ifdef WIFI_PORTAL
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#endif

#ifdef ARDUINO_OTA
#include <ArduinoOTA.h>
#endif

#ifdef HTTP_OTA
// // Define the macro for detailed debug output
// #define DEBUG_ESP_HTTP_UPDATE
// // Redirect debug output to Serial port
// #define DEBUG_ESP_PORT Serial
// #include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#endif

#ifdef JSON_CONFIG_OTA
#include <FS.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h> 
#endif

#ifdef USE_NTP
#include <TZ.h>
#include <coredecls.h> // optional callback to check on server
#endif

#ifdef DEEP_SLEEP_SECONDS
#include <RTCVars.h>
#endif


class BaseApp
{
public:
  BaseApp();
  ~BaseApp();
  void setup();
  void loop();
  
protected:
  // Single client instance that can be either secure or non-secure
  std::unique_ptr<WiFiClient> client;
  // Persisted BearSSL public key used with setKnownKey(); must outlive the secure client
  std::unique_ptr<BearSSL::PublicKey> serverPubKey;
  
  /**
   * @brief Set up the client based on configuration
   * @return true if client was set up successfully, false otherwise
   */
  bool setupClient();
  
  static const uint32_t PROBLEMATIC_FLASH_CHIPS[];  // List of known problematic flash chip IDs
  static const size_t NUM_PROBLEMATIC_FLASH_CHIPS;  // Number of entries in the array
  String FIRMWARE_VERSION = String(__FILE__) + "-" + String(__DATE__) + "-" + String(__TIME__);
  const uint8 WATCHDOG_SETUP_SECONDS = 60; // Setup should complete well within this time limit
  const uint8 WATCHDOG_LOOP_SECONDS = 40;  // Loop should complete well within this time limit
  Ticker watchdog;

  Console console;
  Config config;


  virtual void AppSetup();     // ovveride this if reqired
  virtual void AppLoop();      // ovveride this if reqired
  virtual void AppIntervall(); // ovveride this if reqired
  virtual void AppFirmwareVersion(); // implement in the app class
  virtual const char* getMDNSHostname(); // implement in the app class

  String getResetReasonString(uint8_t reason);
  void timeoutCallback();
  bool connectWiFi();
  void logEnabledFeatures();

  void deepSleep(uint32_t time_us, RFMode mode = RF_DEFAULT);
  
#ifdef USE_MDNS
  void setupMDNS();
#endif

#ifdef WIFI_PORTAL
  // const uint8 WIFI_PORTAL_TRIGGER_PIN = 4; // A low input on this pin will trigger the Wifi Manager Console at boot. Comment out to disable.
  void configModeCallback(WiFiManager *myWiFiManager);
  WiFiManager wifiManager;
#else
  const char *WIFI_SSID = "SSID";
  const char *WIFI_PASSWORD = "password";
#endif

#ifdef CONSOLE_TELNET
  const int TELNET_DEFAULT_PORT = 23;
  TelnetStreamBuffered *pBufferedTelnetStream = nullptr;
#endif

#ifdef CONSOLE_HTTP
  HttpStreamBuffered *pBufferedHTTPRestStream = nullptr;
#endif

#ifdef ARDUINO_OTA
  const int ARDUINO_OTA_PORT = 8266;
  const char *ARDUINO_OTA_PASSWD = "";
  void setupArduinoOta();
#endif

#ifdef HTTP_OTA
  const char *HTTP_OTA_URL = "http://192.168.0.1:8080/firmware";
  const char *HTTP_OTA_USERNAME = "user";
  const char *HTTP_OTA_PASSWORD = "myuserpw";
  void setupHTTPOTA();
  boolean performHttpOtaUpdate();
#endif

#ifdef HTTP_CONFIG
  const char *HTTP_CONFIG_URL = "http://192.168.0.1:8080/config";
  const char *HTTP_CONFIG_USERNAME = "user";
  const char *HTTP_CONFIG_PASSWORD = "myuserpw";
  boolean performHttpConfigUpdate();
#endif // HTTP_CONFIG

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
  boolean ntp_first = true; // first time ntp based event needs processing
  void setupNtp();
  virtual void AppNTPSet();    // called once after NTP time is first set
  void time_is_set(boolean from_sntp /* <= this optional parameter can be used with ESP8266 Core 3.0.0*/);
  uint32_t sntp_startup_delay_MS_rfc_not_less_than_60000();
  uint32_t sntp_update_delay_MS_rfc_not_less_than_15000();
#endif

private:
  uint32_t flashId;
  bool deepSleepWorkaround = false;  // Flag to track if deep sleep workaround is needed
  void deepSleepNK(uint32 time_us); // alternate deep sleep method for zoombie boot issue (see implementation for details)


};

#endif // BASEAPP_H
