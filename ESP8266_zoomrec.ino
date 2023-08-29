/**
 * Program received signal SIGSEGV, Segmentation fault. 0x4000df64 in ?? ()
 *
 *
 * ESP8266 project template with optional:
 *  - WiFi config portal - auto or manual trigger
 *  - OTA update - Arduino or web server
 *  - Deep sleep
 *  - Process timeout watchdog
 *
 * Copyright (c) 2016 Dean Cording  <dean@cording.id.au>
 * Modified by Swapan <swapan@yahoo.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * ESP8266 Pin Connections
 *
 * GPIO0/SPI-CS2/_FLASH_ -                           Pull low for Flash download by UART
 * GPIO1/TXD0/SPI-CS1 -                              _LED_
 * GPIO2/TXD1/I2C-SDA/I2SO-WS -
 * GPIO3/RXD0/I2SO-DATA -
 * GPIO4 -
 * GPIO5/IR-Rx -
 * GPIO6/SPI-CLK -                                  Flash CLK
 * GPIO7/SPI-MISO -                                 Flash DI
 * GPIO8/SPI-MOSI/RXD1 -                            Flash DO
 * GPIO9/SPI-HD -                                   Flash _HD_
 * GPIO10/SPI-WP -                                  Flash _WP_
 * GPIO11/SPI-CS0 -                                 Flash _CS_
 * GPIO12/MTDI/HSPI-MISO/I2SI-DATA/IR-Tx -
 * GPIO13/MTCK/CTS0/RXD2/HSPI-MOSI/I2S-BCK -
 * GPIO14/MTMS/HSPI-CLK/I2C-SCL/I2SI_WS -
 * GPIO15/MTDO/RTS0/TXD2/HSPI-CS/SD-BOOT/I2SO-BCK - Pull low for Flash boot
 * GPIO16/WAKE -
 * ADC -
 * EN -
 * RST -
 * GND -
 * VCC -


/**
 * How to
 * 1) Erase flash: esptool.py --chip ESP8266 -p /dev/ttyUSB0  erase_flash
 * 2) Deep sleep for Wemos D1 Mini: connect D0 with RST using schottky diode (e.g. BAT43, the cathode (ring) towards gpio16/D0 ) 
 *    to cleanly pull rst low without side effects when gpio16 is high. Workaround is just connect via wire
 */

#include <Arduino.h>

#include <stdarg.h>

// char strBuf[128]; // buffer for char string operations
// // can be used like this: println( SNPRINTF("The answer is %d", 42));
// #define SNPRINTF(format, ...) \
//     (snprintf(strBuf, sizeof(strBuf), format, ##__VA_ARGS__) >= 0 ? strBuf : ((strBuf[0] = '\0'), strBuf))

#include "features.h"

#define SERIAL_DEFAULT_BAUD 74880 // native baud rate to see boot message
#define FAST_CONNECTION_TIMEOUT 10000 // timeout for initial connection atempt

#include <ESP8266WiFi.h>
#ifdef WIFI_PORTAL
#include <DNSServer.h>        // Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h> // Local WebServer used to serve the configuration portal
#include <WiFiManager.h>      // https://github.com/tzapu/WiFiManager WiFi Configuration Magic
WiFiManager wifiManager;

// const uint8 WIFI_PORTAL_TRIGGER_PIN = 4; // A low input on this pin will trigger the Wifi Manager Console at boot. Comment out to disable.
#else
const char *WIFI_SSID = "SSID" const char *WIFI_PASSWORD = "password"
#endif

#ifdef TELNET
// requires mofification in TelnetStream.cpp to change scope from private to protected
// for TelnetStreamBuffered class so access isConnected method
#include "TelnetStreamBuffered.h"
#define TELNET_DEFAULT_PORT 23
#endif

#include "console.h"
Console console;

#ifdef ARDUINO_OTA
/* Over The Air updates directly from Arduino IDE */
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define ARDUINO_OTA_PORT 8266
// #define ARDUINO_OTA_HOSTNAME  "esp8266"
#define ARDUINO_OTA_PASSWD "1csqBgpHchquXkzQlgl4"
#endif

#ifdef HTTP_OTA
/* Over The Air automatic firmware update from a web server.  ESP8266 will contact the
 *  server on every boot and check for a firmware update.  If available, the update will
 *  be downloaded and installed.  Server can determine the appropriate firmware for this
 *  device from any combination of HTTP_OTA_VERSION, MAC address, and firmware MD5 checksums.
 */
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
// default: can be overwritten by config file
#define HTTP_OTA_URL "http://192.168.0.1:8080/firmware"
#define HTTP_OTA_USERNAME "admin"
#define HTTP_OTA_PASSWORD "myadmin"
// string describing current version
#define HTTP_OTA_VERSION String(__FILE__) + "-" + String(__DATE__) + "-" + String(__TIME__)
#endif

// Sample configuration file
// {
//     "http_api_username": "test",
//     "http_api_password": "test123",
//     "http_api_base_url": "https://dog.ceo",
//     "timezone_ntp" : "AEST-10AEDT,M10.1.0,M4.1.0/3",
//     "http_ota_url": "http://192.168.0.237:8080/firmware",
//     "http_ota_username": "myuser",
//     "http_ota_password": "mypassword",
//     "json_config_ota_username": "admin",
//     "json_config_ota_password": "admin123",
//     "json_config_ota_port": 8080,
//     "json_config_ota_path": "/config",
//     "telnet_port": 23   
// }
#ifdef JSON_CONFIG_OTA
#include <FS.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>


#include "config.h"
Config config;

/* sample config.json file to upload
curl -v  -X POST  -H "Content-Type: application/json" -u admin:myadminpw --data @ESP8266_Template.config.json http:/<IP of ESP8266>:8080/config
{
    "serial_baud" : 115200,
    "http_api_username": "testuser",
    "http_api_password": "test123",
    "http_api_base_url": "https://dog.ceo",
    "timezone_ntp" : "AEST-10AEDT,M10.1.0,M4.1.0/3",
    "http_ota_url": "http://<IP of HTTP server>:8080/firmware",
    "http_ota_username": "myuser",
    "http_ota_password": "myadminpw",
    "json_config_ota_username": "admin",
    "json_config_ota_password": "myadminpw",
    "json_config_ota_port": 8080,
    "json_config_ota_path": "/config",
    "telnet_port": 23   
}
*/


#define JSON_CONFIG_OTA_FILE "/config.json"
#endif

const uint8 STATUS_LED = LED_BUILTIN; // Built-in blue LED change if required e.g. pin 2
const char *SSID = (String("ESP") + String(ESP.getChipId())).c_str();

/* Watchdog to guard against the ESP8266 wasting battery power looking for
 *  non-responsive wifi networks and servers. Expiry of the watchdog will trigger
 *  either a deep sleep cycle or a delayed reboot. The ESP8266 OS has another built-in
 *  watchdog to protect against infinite loops and hangups in user code.
 */
#include <Ticker.h>
Ticker watchdog;
const uint8 WATCHDOG_SETUP_SECONDS = 30; // Setup should complete well within this time limit
const uint8 WATCHDOG_LOOP_SECONDS = 20;  // Loop should complete well within this time limit

void timeout_cb()
{
  // This sleep happened because of timeout. Do a restart after a sleep
  console.log(Console::CRITICAL, F("Watchdog timeout...restarting"));
  console.flush();

#ifdef DEEP_SLEEP_SECONDS
  // Enter DeepSleep so that we don't exhaust our batteries by countinuously trying to
  // connect to a network that isn't there.
  ESP.deepSleep(DEEP_SLEEP_SECONDS * 1000, WAKE_RF_DEFAULT);
  // Do nothing while we wait for sleep to overcome us
  while (true)
  {
  };

#else
  delay(1000);
  ESP.restart();
#endif
}

#ifdef TIMER_INTERVAL_MILLIS
unsigned long timer_interval = 0;
#endif

#ifdef DEEP_SLEEP_SECONDS
unsigned long timer_startup = 0; // 
#endif

#ifdef LED_STATUS_FLASH

#define LED_ON LOW   // Define the value to turn the LED on
#define LED_OFF HIGH // Define the value to turn the LED off

Ticker flasher;

void flash()
{
  digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
}
#endif

#ifdef WIFI_PORTAL
// Callback for entering config mode
void configModeCallback(WiFiManager *myWiFiManager)
{
  // Config mode has its own timeout
  watchdog.detach();

#ifdef LED_STATUS_FLASH
  flasher.attach(0.2, flash);
#endif
}
#endif

#ifdef GDB_DEBUG
#include <GDBStub.h>
#endif

#ifdef JSON_CONFIG_OTA

#endif

#ifdef USE_NTP
#include <TZ.h>
#include <coredecls.h> // optional callback to check on server

// from https://werner.rothschopf.net/202011_arduino_esp8266_ntp_en.htm
#define NTP_SERVER "pool.ntp.org"
// Timezone definition https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
#define NTP_DEFAULT_TIME_ZONE TZ_Europe_London

boolean ntp_set = false; // has ntp time been set

void time_is_set(boolean from_sntp /* <= this optional parameter can be used with ESP8266 Core 3.0.0*/)
{
  ntp_set = true;

  
  time_t now = time(nullptr);
  console.log(Console::INFO, F("NTP time received from_sntp=%s"), from_sntp ? "true" : "false");
  console.log(Console::INFO, F("Current local time: %s"), ctime(&now));
}

uint32_t sntp_startup_delay_MS_rfc_not_less_than_60000()
{
  randomSeed(A0);
  return random(5000);
}

uint32_t sntp_update_delay_MS_rfc_not_less_than_15000()
{
  return 12 * 60 * 60 * 1000UL; // 12 hours
}
#endif

#ifdef HTTPS_REST_CLIENT
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <UrlEncode.h> // https://github.com/plageoj/urlencode

// Define a function to send an HTTPS request with basic authentication
DynamicJsonDocument performHttpsRequest(const char *method, const char *url, const char *path,
                                        const char *http_username, const char *http_password, const char *tls_fingerprint, size_t response_capacity = 1024,
                                        DynamicJsonDocument *requestBody = nullptr)
{
  WiFiClient client;

  if (strncmp(url, "https", 5) == 0) {
    // Use WiFiClientSecure for HTTPS requests
    WiFiClientSecure secureClient;
  
    if (strlen(tls_fingerprint) > 0)
      secureClient.setFingerprint(tls_fingerprint);
    else
      secureClient.setInsecure();

    client = secureClient;
  }

  // Initialize the HTTP client with the WiFi client and server/port
  HTTPClient http;
  String uri = String(url) + String(path);
  http.begin(client, uri);

  console.log(Console::DEBUG, F("HTTP Rest Request: %s %s"), method, uri.c_str());

  // Set the HTTP request headers with the basic authentication credentials
  String auth = String(http_username) + ":" + String(http_password);
  String encodedAuth = base64::encode(auth);
  http.setAuthorization(encodedAuth);

  // Send the HTTP request to the API endpoint
  int httpCode;
  if (String(method) == "GET")
  {
    httpCode = http.GET();
  }
  else if (String(method) == "POST")
  {
    // Serialize the JSON request body into a string
    String requestBodyStr;
    serializeJson(*requestBody, requestBodyStr);

    // Set the HTTP request headers for a JSON POST request
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Content-Length", String(requestBodyStr.length()));

    // Send the HTTP POST request with the JSON request body
    httpCode = http.POST(requestBodyStr);
  }
  else
  {
    // Invalid HTTP method
    httpCode = -99; // not official code
  }

  DynamicJsonDocument response(response_capacity);

  if (httpCode == HTTP_CODE_OK)
  {
    // Read the response JSON data into a DynamicJsonDocument
    DeserializationError error = deserializeJson(response, http.getString());

    if (error)
    {
      console.log(Console::ERROR, F("Failed to parse response JSON: %s"), error.c_str());
    }
  }
  else
  {
    console.log(Console::ERROR, F("HTTP error: (%d) %s"), httpCode, http.errorToString(httpCode).c_str());
  }

  // Release the resources used by the HTTP client
  http.end();

  return response;
}

#endif

#ifdef ARDUINO_OTA
void setup_ArduinoOTA()
{
  // Arduino OTA Initalisation
  ArduinoOTA.setPort(ARDUINO_OTA_PORT);
  ArduinoOTA.setHostname(SSID);
  ArduinoOTA.setPassword(ARDUINO_OTA_PASSWD);
  ArduinoOTA.onStart([]()
                     {
      watchdog.detach();
      console.log(Console::INFO, F("OTA upload starting...")); });
  ArduinoOTA.onEnd([]()
                   { console.log(Console::INFO, F("OTA upload finished")); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { console.log(Console::INFO, F("Progress: %u%%"), (progress / (total / 100))); });
  ArduinoOTA.onError([](ota_error_t error)
                     {
      console.log(Console::ERROR, F("Error[%u]: "), error);
      if (error == OTA_AUTH_ERROR) console.log(Console::ERROR, F("Auth Failed"));
      else if (error == OTA_BEGIN_ERROR) console.log(Console::ERROR, F("Begin Failed"));
      else if (error == OTA_CONNECT_ERROR) console.log(Console::ERROR, F("Connect Failed"));
      else if (error == OTA_RECEIVE_ERROR) console.log(Console::ERROR, F("Receive Failed"));
      else if (error == OTA_END_ERROR) console.log(Console::ERROR, F("End Failed")); });
  ArduinoOTA.begin();
}
#endif

#ifdef HTTP_OTA
boolean perform_HTTP_OTA_Update()
{
  String http_ota_url = useConfig("http_ota_url", HTTP_OTA_URL);
  String http_ota_username = useConfig("http_ota_username", HTTP_OTA_USERNAME);
  String http_ota_password = useConfig("http_ota_password", HTTP_OTA_PASSWORD);

  // Check server for firmware updates
  console.log(Console::INFO, F("Checking for firmware updates from %s"), http_ota_url.c_str());

  WiFiClient client;
#ifdef LED_STATUS_FLASH
  ESPhttpUpdate.setLedPin(STATUS_LED, LED_ON); // define level for LED on
#endif

  // use authorization?
  if (!http_ota_username.isEmpty() && !http_ota_password.isEmpty())
    ESPhttpUpdate.setAuthorization(http_ota_username, http_ota_password);

  switch (ESPhttpUpdate.update(client, http_ota_url, HTTP_OTA_VERSION))
  {
  case HTTP_UPDATE_FAILED:
    console.log(Console::WARNING, F("HTTP update failed: Code (%d) %s"), ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
    return false;

  case HTTP_UPDATE_NO_UPDATES:
    console.log(Console::INFO, F("No updates"));
    return false;

  case HTTP_UPDATE_OK:
    console.log(Console::INFO, F("Update OK"));
    return true;
  default:
    return false;
  }
  return false;
}
#endif

#ifdef USE_NTP
void setup_NTP()
{
  settimeofday_cb(time_is_set); // optional: callback if time was sent
  configTime( config.useConfig("timezone_ntp", NTP_DEFAULT_TIME_ZONE), NTP_SERVER);
}
#endif

#ifdef JSON_CONFIG_OTA
ESP8266WebServer server(JSON_CONFIG_OTA_PORT);


#endif

boolean connectWIFI()
{
#ifdef LED_STATUS_FLASH
  pinMode(STATUS_LED, OUTPUT);
  flasher.attach(0.6, flash);
#endif

  // Watchdog timer - resets if setup takes longer than allocated time
  watchdog.once(WATCHDOG_SETUP_SECONDS, &timeout_cb);

  // try connect using previous connection details stored in eeprom
  if (WiFi.SSID().length() > 0)
  {
    // Print the SSID and password
    console.log(Console::INFO, F("WiFi credentials stored: %s"), WiFi.SSID().c_str());

    console.log(Console::INFO, F("Connecting..."));
    
    WiFi.begin(WiFi.SSID(), WiFi.psk());
    WiFi.waitForConnectResult(FAST_CONNECTION_TIMEOUT);
  }

#ifdef WPS_CONFIG
  if (WiFi.status() != WL_CONNECTED)
  {
    console.log(Console::INFO, F("Starting WPS configuration..."));
    WiFi.beginWPSConfig();

    if (WiFi.SSID().length() > 0)
    {
      // Print the SSID and password
      console.printf("WPS credentials received: %s", WiFi.SSID().c_str());

      WiFi.begin(WiFi.SSID(), WiFi.psk());
      WiFi.persistent(true);
      WiFi.setAutoConnect(true);
      WiFi.setAutoReconnect(true);
      if (WiFi.waitForConnectResult() != WL_CONNECTED)
      {
        console.log(Console::WARNING, F("Connecting using WPS timed out!"));
        timeout_cb();
      }
    }
  }
#endif

  // Set up WiFi connection using portal
  if (WiFi.status() != WL_CONNECTED)
  {
#ifdef WIFI_PORTAL
#ifdef WIFI_PORTAL_TRIGGER_PIN
    pinMode(WIFI_PORTAL_TRIGGER_PIN, INPUT_PULLUP);
    delay(100);
    if (digitalRead(WIFI_PORTAL_TRIGGER_PIN) == LOW)
    {
      watchdog.detach();
      if (!wifiManager.startConfigPortal(SSID, NULL))
      {
        console.log(Console::ERROR, F("Starting WiFi Config Portal Failed!"));
        timeout_cb();
      }
    }
    else
    {
#endif

      // for debugging
      wifiManager.setDebugOutput(true);

      wifiManager.setConfigPortalTimeout(180);
      wifiManager.setAPCallback(configModeCallback);
      if (!wifiManager.autoConnect())
      {
        console.log(Console::WARNING, F("Connection Failed!"));
        timeout_cb();
      }

#ifdef WIFI_PORTAL_TRIGGER_PIN
    }
#endif

#else
    // Save boot up time by not configuring them if they haven't changed
    if (WiFi.SSID() != WIFI_SSID)
    {
      console.log(Console::INFO, F("Initialising Wifi..."));
      WiFi.mode(WIFI_STA);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      WiFi.persistent(true);
      WiFi.setAutoConnect(true);
      WiFi.setAutoReconnect(true);
    }
#endif
  }

  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    console.log(Console::WARNING, F("Connection Finally Failed!"));
    timeout_cb();
  }

#ifdef LED_STATUS_FLASH
  flasher.detach();
  digitalWrite(STATUS_LED, LED_OFF);
#endif

  if (WiFi.status() == WL_CONNECTED)
  {
    // we have internet connection
    console.log(Console::INFO, F("IP address: %s"), WiFi.localIP().toString().c_str());
    return true;
  }
  else
    return false;
}

String getResetReasonString(uint8_t reason) {
  switch (reason) {
    case REASON_DEFAULT_RST:    return "Power-on reset";
    case REASON_WDT_RST:        return "Watchdog reset";
    case REASON_EXCEPTION_RST:  return "Exception reset";
    case REASON_SOFT_WDT_RST:   return "Software Watchdog reset";
    case REASON_SOFT_RESTART:   return "Software restart";
    case REASON_DEEP_SLEEP_AWAKE: return "Deep sleep wake-up";
    case REASON_EXT_SYS_RST:    return "External system reset";
    default:                    return "Unknown reset reason";
  }
}


// Put any project specific code here
const int inputPinResetSwitch = D1;
const int outputPinPowerButton = D2;
int lastStatus = LOW;

void setup()
{
#ifdef GDB_DEBUG
  gdbstub_init();
#endif

  // Initialize SPIFFS and read config
  if (!SPIFFS.begin()) {
    console.begin( SERIAL_DEFAULT_BAUD);
    console.log(Console::CRITICAL, F("Failed to initialize SPIFFS for config!"));
  }
  else {
    retrieveJSON(config, JSON_CONFIG_OTA_FILE);

    // try to initialize with baud rate from config
    int serial_baud = config["serial_baud"].as<int>();
    serial_baud = (serial_baud) ? serial_baud : SERIAL_DEFAULT_BAUD; 
    console.begin( serial_baud); 

    // determine logLevel
    int logLevel = config["log_level"].as<int>();
    console.setLogLevel( console.intToLogLevel( logLevel));
  }
  console.println(); // newline after garbage from startup
  console.log(Console::DEBUG, F("Start of initialization: Reset Reason='%s'"), getResetReasonString(ESP.getResetInfoPtr()->reason).c_str()); 

  // setup for deep sleep
  pinMode(D0, WAKEUP_PULLUP); // be carefull when using D0 and using deep sleep
  int deep_sleep_option = config["deep_sleep_option"].as<int>();
  deep_sleep_option = (deep_sleep_option) ? deep_sleep_option : WAKE_RFCAL; 
  system_deep_sleep_set_option( deep_sleep_option);

  // connect to WIFI depending on what connection features are enabled
  connectWIFI();

#ifdef TELNET
  int port = config["telnet_port"].as<int>();
  port = (port) ? port : TELNET_DEFAULT_PORT; 
  console.log(Console::INFO, F("Telnet service started on port: %d"), port);
  BufferedTelnetStream.begin(port);
  console.begin(BufferedTelnetStream, Serial); // continue output to Serial
#endif

  // Print config
  if (config != nullptr)
  {
    serializeJsonPretty(config, console);
    console.println();
  }

#ifdef HTTP_OTA
  // remove watchdog while updating
  watchdog.detach();
  perform_HTTP_OTA_Update();
  // Watchdog timer - resets if setup takes longer than allocated time
  watchdog.once(WATCHDOG_SETUP_SECONDS, &timeout_cb);
#endif

#ifdef ARDUINO_OTA
  setup_ArduinoOTA();
#endif

#ifdef JSON_CONFIG_OTA
  setup_JSON_CONFIG_OTA();
#endif

#ifdef USE_NTP
  setup_NTP();
#endif

#ifdef HTTP_OTA
  perform_HTTP_OTA_Update();
#endif

  console.log(Console::INFO, F("Current firmware version: '%s'"), (HTTP_OTA_VERSION).c_str());

  // Put your initialisation code here
  pinMode(inputPinResetSwitch, INPUT_PULLUP);
  pinMode(outputPinPowerButton, OUTPUT);

  console.log(Console::DEBUG, F("End of initialization"));
  watchdog.detach();
}

time_t convertISO8601ToUnixTime(const char* isoTimestamp) {
  struct tm timeinfo;
  memset(&timeinfo, 0, sizeof(timeinfo));

  // Parse the ISO8601 timestamp
  sscanf(isoTimestamp, "%4d-%2d-%2dT%2d:%2d:%2d",
         &timeinfo.tm_year, &timeinfo.tm_mon, &timeinfo.tm_mday,
         &timeinfo.tm_hour, &timeinfo.tm_min, &timeinfo.tm_sec,
         &timeinfo.tm_hour, &timeinfo.tm_min);

  // Adjust year and month for struct tm format
  timeinfo.tm_year -= 1900;
  timeinfo.tm_mon -= 1;

  // Convert to Unix time
  time_t unixTime = mktime(&timeinfo);

  return unixTime;
}

const char* unixTimestampToISO8601(time_t unixTimestamp, char* iso8601Time, size_t bufferSize) {
  struct tm *tmStruct = localtime(&unixTimestamp);
  snprintf(iso8601Time, bufferSize, "%04d-%02d-%02dT%02d:%02d:%02d",
           tmStruct->tm_year + 1900, tmStruct->tm_mon + 1, tmStruct->tm_mday,
           tmStruct->tm_hour, tmStruct->tm_min, tmStruct->tm_sec);
  return iso8601Time;
}

bool isPCRunning() {
  return digitalRead( inputPinResetSwitch) == HIGH;
}

void togglePowerButton() {
    digitalWrite(outputPinPowerButton, HIGH);
    delay(150);
    digitalWrite(outputPinPowerButton, LOW);
}

void startPC() {
  if (!isPCRunning()) 
    togglePowerButton();
}

void shutDownPC() {
   if (isPCRunning()) 
    togglePowerButton();
}

void run_demo()
{

  console.log(Console::DEBUG, F("PC status: %s"), isPCRunning() ? "true" : "false");

#ifdef HTTPS_REST_CLIENT
  // test http client
  if (!hasConfig("http_api_base_url") || (!hasConfig("timezone"))) {
    console.log(Console::CRITICAL, F("Config 'http_api_base_url' or 'timezone' missing."));
  }
  else {
    // API route
    char path[100];
    snprintf(path, sizeof(path), "/event/next?astimezone=%s&leadinsecs=%d&leadoutsecs=%d", 
      urlEncode( useConfig("timezone", "")).c_str(), useConfig("leadin_secs", 60), useConfig("leadout_secs", 60));

    DynamicJsonDocument response = performHttpsRequest("GET", useConfig("http_api_base_url", ""), path, 
      config["http_api_username"], config["http_api_password"], "");

    bool eventOngoing = false;
    if (response.isNull()) {
      console.log(Console::DEBUG, F("response for /event/next is empty"));
    }
    else {
      serializeJsonPretty(response, console);
      console.println();
      
      // extract
      char startStr[30];
      char endStr[30];

      strcpy ( startStr, response["start_astimezone"]);
      strcpy ( endStr, response["end_astimezone"]);     

      if ((startStr == nullptr || *startStr == '\0') || 
          (endStr == nullptr || *endStr == '\0')) {
        console.log(Console::DEBUG, F("response does not contain start_astimezone or end_astimezone"));
      }
      else {
        // Convert ISO 8601 timestamps to time_t (Unix timestamp)
        time_t startTime = convertISO8601ToUnixTime(startStr);
        time_t endTime = convertISO8601ToUnixTime(endStr);
        
        // Get current UTC time
        time_t currentTime = time(nullptr);
        
        // Check if the event has started and has not yet ended
        if ((currentTime >= startTime) && (currentTime <= endTime)) 
          eventOngoing = true;
      }
    }
    
    if (eventOngoing) {
      console.log(Console::DEBUG, F("Event ongoing..."));
      if (!isPCRunning()) {
        startPC();
        console.log(Console::DEBUG, F("Starting PC..."));
      }
    } 
    else {
      console.log(Console::DEBUG, F("No event ongoing..."));
      if (isPCRunning()) {
        shutDownPC();
        console.log(Console::DEBUG, F("Shutting down PC..."));
      }
    }
  }
#endif

  console.log(Console::DEBUG, F("Free heap: %d Max Free Block: %d"), ESP.getFreeHeap(), ESP.getMaxFreeBlockSize());
}

#ifdef TIMER_INTERVAL_MILLIS

void executeTimerCode()
{
  // put your main code here, to run repeatedly usinf non blocking timer
  run_demo();
}

#endif

void loop()
{

#ifdef ARDUINO_OTA
  // Handle any OTA upgrade
  ArduinoOTA.handle();
#endif

#ifdef JSON_CONFIG_OTA
  // Handle HTTP requests
  server.handleClient();
#endif

  // Watchdog timer - resets if setup takes longer than allocated time
  watchdog.once(WATCHDOG_LOOP_SECONDS, &timeout_cb);

#ifdef USE_NTP
  // do not start processing main loop until NTP time is set
  if (ntp_set) {

#ifdef TIMER_INTERVAL_MILLIS
  // https://www.norwegiancreations.com/2018/10/arduino-tutorial-avoiding-the-overflow-issue-when-using-millis-and-micros/
  if ((unsigned long)(millis() - timer_interval) > TIMER_INTERVAL_MILLIS)
  {
    timer_interval = millis();
    executeTimerCode();
  }
#else
  // put your main code here, to run repeatedly:

#endif

  watchdog.detach();

#ifdef DEEP_SLEEP_SECONDS

  // no deep sleep after normal power up to allow for OTA updates
  bool expired = false;
  if ((unsigned long)(millis() - timer_startup) > DEEP_SLEEP_STARTUP_SECONDS * 1000) {
    timer_startup = millis();
    expired = true;
  }

  if ((ESP.getResetInfoPtr()->reason == REASON_DEEP_SLEEP_AWAKE) || expired) {
    // Enter DeepSleep
    console.log(Console::DEBUG, F("Entering deep sleep for %d seconds..."), DEEP_SLEEP_SECONDS);
    ESP.deepSleep(DEEP_SLEEP_SECONDS * 1000000, WAKE_RF_DEFAULT);
    // Do nothing while we wait for sleep to overcome us
    while (true)
    {
    };
  }
#endif
  }
#endif
}