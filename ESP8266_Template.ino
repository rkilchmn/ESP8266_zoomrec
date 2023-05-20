/**
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
 */

#include <Arduino.h>

#include <stdarg.h>

// char strBuf[128]; // buffer for char string operations
// // can be used like this: println( SNPRINTF("The answer is %d", 42));
// #define SNPRINTF(format, ...) \
//     (snprintf(strBuf, sizeof(strBuf), format, ##__VA_ARGS__) >= 0 ? strBuf : ((strBuf[0] = '\0'), strBuf))

// Optional functionality. Comment out defines to disable feature
#define WIFI_PORTAL      // Enable WiFi config portal
#define WPS_CONFIG       // press WPS bytton on wifi pathr
#define ARDUINO_OTA      // Enable Arduino IDE OTA updates
#define HTTP_OTA         // Enable OTA updates from http server
#define LED_STATUS_FLASH // Enable flashing LED status
// #define DEEP_SLEEP_SECONDS  300       // Define for sleep timer_interval between process repeats. No sleep if not defined
#define TIMER_INTERVAL_MILLIS 5000 // periodically execute code using non-blocking timer instead delay()
#define JSON_CONFIG_OTA   // upload JSON config via OTA providing REST API
#define GDB_DEBUG         // enable debugging using GDB using serial
#define USE_NTP           // enable NTP
#define HTTPS_REST_CLIENT // provide HTTPS REST client
#define TELNET            // use telnet

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

class Console : public Stream {
private:
  Stream* currentStream;

public:
  // Constructor
  Console() {
    currentStream = &Serial;
  }

  // Required Stream functions to implement
  virtual size_t write(uint8_t data) {
    // Print the data to the current stream
    return currentStream->write(data);
  }

  virtual int available() {
    // Check the availability of input data on the current stream
    return currentStream->available();
  }

  virtual int read() {
    // Read a byte of data from the current stream
    return currentStream->read();
  }

  virtual int peek() {
    // Peek at the next byte of input data from the current stream
    return currentStream->peek();
  }

  // Additional functions from Stream that can be implemented if needed
  // ...

  // Function to begin with Serial
  void begin(unsigned long baudRate) {
    Serial.begin(baudRate);
    currentStream = &Serial;
  }

  // Function to begin with a stream e.g. TelnetStream
  void begin(Stream& stream) {
    currentStream = &stream;
  }

  void log(const __FlashStringHelper *format, ...)
  {
    const char *fmt = reinterpret_cast<const char *>(format);

    time_t localTime = time(nullptr);
    struct tm *tm = localtime(&localTime);

    // Create a temp string using strftime() function
    char temp[100];
    strftime(temp, sizeof(temp), "%Y-%m-%d %H:%M:%S ", tm);

    currentStream->print(temp);
    va_list arg;
    va_start(arg, format);
    vsnprintf(temp, sizeof(temp), fmt, arg);
    va_end(arg);
    currentStream->print(temp);
    currentStream->print("\n");
  }

  // // Function to log messages
  // template<typename... Args>
  // void log(const Args&... args) {
  //   printTimestamp();
  //   print(args...);
  //   println();
  // }

};

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
#define HTTP_OTA_USERNAME "myupdateuser"
#define HTTP_OTA_PASSWORD "myupdatepw"
// string describing current version
#define HTTP_OTA_VERSION String(__FILE__) + "-" + String(__DATE__) + "-" + String(__TIME__)
#endif

#ifdef JSON_CONFIG_OTA
#include <FS.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

#define JSON_CONFIG_MAXSIZE 4096 // initial size to allocate, resized later what is required

// Checkout this how to access config: https://arduinojson.org/v6/example/string/
DynamicJsonDocument config(0); // will be resized when rwad from file

/* sample config.json file to upload
curl -v  -X POST  -H "Content-Type: application/json" -u admin:mypassword --data @config.json http://192.168.0.30/config
{
  "http_username": "testuser",
  "http_password": "test123",
  "hostname": "dog.ceo",
  "port": 443,
  "timezone" : "AEST-10AEDT,M10.1.0,M4.1.0/3"
}
*/
// defaults - can be overwritten with JSON config file
#define JSON_CONFIG_USERNAME "myJsonUser"
#define JSON_CONFIG_PASSWD "myJsonPassword"
#define JSON_CONFIG_OTA_PATH "/config"
#define JSON_CONFIG_OTA_PORT 80

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
  console.println(F("Watchdog timeout..."));

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
  unsigned long timer_now = 0;
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
// return true on success
boolean retrieveJSON(DynamicJsonDocument &doc, String filename)
{
  // read the JSON file from SPIFFS
  File file = SPIFFS.open(filename, "r");
  if (!file)
  {
    return false;
  }

  doc = DynamicJsonDocument(JSON_CONFIG_MAXSIZE);
  // parse the JSON file into the JSON object
  DeserializationError error = deserializeJson(doc, file);
  doc.shrinkToFit();
  file.close();

  return !error;
}

const char *useConfig( const char *configKey, const char *defaultValue)
{
  if (!config.isNull() && config.containsKey(configKey))
    return config[configKey];
  else
    return defaultValue;
}

String useConfig(const char *configKey, String defaultValue)
{
  if (!config.isNull() && config.containsKey(configKey))
    return config[configKey].as<String>();
  else
    return defaultValue;
}

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

  console.println("time was sent! from_sntp=" + from_sntp);
  time_t now = time(nullptr);
  console.println(ctime(&now));
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

// Define a function to send an HTTPS request with basic authentication
DynamicJsonDocument performHttpsRequest(const char *method, const char *url, const char *path,
                                        const char *http_username, const char *http_password, const char *tls_fingerprint, size_t response_capacity = 1024,
                                        DynamicJsonDocument *requestBody = nullptr)
{

  // Initialize the WiFi client with SSL/TLS support
  WiFiClientSecure client;
  if (strlen(tls_fingerprint) > 0)
    client.setFingerprint(tls_fingerprint);
  else
    client.setInsecure();

  // Initialize the HTTP client with the WiFi client and server/port
  HTTPClient http;
  http.begin(client, String(url) + String(path));

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
    httpCode = -1;
  }

  DynamicJsonDocument response(response_capacity);

  if (httpCode == HTTP_CODE_OK)
  {
    // Read the response JSON data into a DynamicJsonDocument
    DeserializationError error = deserializeJson(response, http.getString());

    if (error)
    {
      console.println("Failed to parse response JSON: " + String(error.c_str()));
    }
  }
  else
  {
    console.println("HTTP error code: " + httpCode);
  }

  // Release the resources used by the HTTP client
  http.end();

  return response;
}

#endif

#ifdef TELNET
#include <TelnetStream.h>
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
      console.println(F("Start")); });
  ArduinoOTA.onEnd([]()
                   { console.println(F("\nEnd")); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { console.printf("Progress: %u%%\r", (progress / (total / 100))); });
  ArduinoOTA.onError([](ota_error_t error)
                     {
      console.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) console.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) console.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) console.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) console.println("Receive Failed");
      else if (error == OTA_END_ERROR) console.println("End Failed"); });
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
  console.log(F("Checking for firmware updates from %s"), http_ota_url.c_str());

  WiFiClient client;
#ifdef LED_STATUS_FLASH
  ESPhttpUpdate.setLedPin(STATUS_LED, LED_ON); // define level for LED on
#endif

  // use authorization?
  if (!http_ota_username.isEmpty() && !http_ota_password.isEmpty())
    ESPhttpUpdate.setAuthorization( http_ota_username, http_ota_password);

  switch (ESPhttpUpdate.update(client, http_ota_url, HTTP_OTA_VERSION))
  {
  case HTTP_UPDATE_FAILED:
    console.log(F("HTTP update failed: Error (%d): %s\n"), ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
    return false;

  case HTTP_UPDATE_NO_UPDATES:
    console.log(F("No updates"));
    return false;

  case HTTP_UPDATE_OK:
    console.log(F("Update OK"));
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
  if (!config.isNull() && config.containsKey("timezone"))
    configTime(config["timezone"], NTP_SERVER);
  else
    configTime(NTP_DEFAULT_TIME_ZONE, NTP_SERVER);
}
#endif

#ifdef JSON_CONFIG_OTA
ESP8266WebServer server(JSON_CONFIG_OTA_PORT);

void handleConfig()
{
  if (!server.authenticate( useConfig( "json_config_ota_username", JSON_CONFIG_USERNAME), 
                            useConfig( "json_config_ota_password", JSON_CONFIG_PASSWD)))
  {
    return server.requestAuthentication();
  }

  if (server.method() == HTTP_POST)
  {
    // Only accept JSON content type
    if (server.hasHeader("Content-Type") && server.header("Content-Type") == "application/json")
    {
      // Parse JSON payload
      DynamicJsonDocument doc(JSON_CONFIG_MAXSIZE);
      deserializeJson(doc, server.arg("plain"));
      doc.shrinkToFit();

      // Store JSON payload in SPIFFS
      File configFile = SPIFFS.open(JSON_CONFIG_OTA_FILE, "w");
      if (configFile)
      {
        serializeJson(doc, configFile);
        configFile.close();
        retrieveJSON(config, JSON_CONFIG_OTA_FILE); // refresh config
        server.send(200);
      }
      else
      {
        server.send(500, "text/plain", "Failed to open config file for writing");
      }
    }
    else
    {
      server.send(400, "text/plain", "Invalid content type: " + server.header("Content-Type"));
    }
  }
  else
  {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

void setup_JSON_CONFIG_OTA()
{
  // Initialize SPIFFS
  if (!SPIFFS.begin())
  {
    console.println("Failed to initialize SPIFFS");
  }

  // Handle HTTP POST request for config
  server.on( useConfig( "json_config_ota_path", JSON_CONFIG_OTA_PATH), handleConfig);

  // list of headers to be parsed
  const char *headerkeys[] = {"Content-Type"};
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char *);
  // ask server to parsed these headers
  server.collectHeaders(headerkeys, headerkeyssize);

  // Start server
  JsonVariant v = config["json_config_ota_port"]; 
  int port = v.as<int>();
  // not working: port = 0
  console.printf( "json_config_ota_port=%d\n", port);
  if (port)
    server.begin( port);
  else
    server.begin();
}
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
    console.printf("WiFi credentials stored: %s\n", WiFi.SSID().c_str());

    console.println(F("Connecting..."));
    WiFi.begin(WiFi.SSID(), WiFi.psk());
    WiFi.waitForConnectResult(FAST_CONNECTION_TIMEOUT);
  }

#ifdef WPS_CONFIG
  if (WiFi.status() != WL_CONNECTED)
  {
    console.println(F("Starting WPS configuration..."));
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
        console.println(F("Connecting using WPS failed!"));
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
        console.println(F("Config Portal Failed!"));
        timeout_cb();
      }
    }
    else
    {
#endif

      wifiManager.setConfigPortalTimeout(180);
      wifiManager.setAPCallback(configModeCallback);
      if (!wifiManager.autoConnect())
      {
        console.println(F("Connection Failed!"));
        timeout_cb();
      }

#ifdef WIFI_PORTAL_TRIGGER_PIN
    }
#endif

#else
    // Save boot up time by not configuring them if they haven't changed
    if (WiFi.SSID() != WIFI_SSID)
    {
      console.println(F("Initialising Wifi..."));
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
    console.println(F("Connection Finally Failed!"));
    timeout_cb();
  }

#ifdef LED_STATUS_FLASH
  flasher.detach();
  digitalWrite(STATUS_LED, LED_OFF);
#endif

  if (WiFi.status() == WL_CONNECTED)
  {
    // we have internet connection
    console.print(F("IP address: "));
    console.println(WiFi.localIP());
    return true;
  }
  else
    return false;
}

// Put any project specific initialisation here

void setup()
{
#ifdef GDB_DEBUG
  gdbstub_init();
#endif

  console.begin(115200);

  console.println(F("Booting"));

  // connect to WIFI depending on what connection features are enabled
  connectWIFI();

#ifdef TELNET
  console.println(F("Console output switching to Telnet!"));
  TelnetStream.begin();
  console.begin( TelnetStream) ;
#else

#endif

#ifdef HTTP_OTA
  perform_HTTP_OTA_Update();
#endif

#ifdef ARDUINO_OTA
  setup_ArduinoOTA();
#endif

#ifdef JSON_CONFIG_OTA
  setup_JSON_CONFIG_OTA();
#endif

#ifdef JSON_CONFIG_OTA
  if (retrieveJSON(config, JSON_CONFIG_OTA_FILE))
  {
    serializeJsonPretty(config, console);
    console.println();
  }
#endif

#ifdef USE_NTP
  setup_NTP();
#endif

  console.log(F("Current firmware version: %s"), (HTTP_OTA_VERSION).c_str());

  // Put your initialisation code here

  console.println(F("Ready"));
  watchdog.detach();
}

void run_demo() {
#ifdef HTTPS_REST_CLIENT
  // test http client
  if (!config.isNull() && config.containsKey("http_api_base_url"))
  {
    // call API
    DynamicJsonDocument response = performHttpsRequest("GET", config["http_api_base_url"],
                                                       "/api/breeds/image/random", config["http_api_username"], config["http_api_password"], "");

    serializeJsonPretty(response, console);
    console.println();
  }
#endif

#ifdef USE_NTP
  if (ntp_set)
  {
    // ISO 8601 timestamp
    String timestampStr = "2023-05-01T14:30:00+02:00";
    // Get current UTC time
    time_t now = time(nullptr);
    struct tm *tmNow = gmtime(&now);

    // Get current local time
    time_t localNow = mktime(tmNow);
    struct tm *tmLocal = localtime(&localNow);

    // Calculate time zone offset in seconds
    int timeZone = (tmLocal->tm_hour - tmNow->tm_hour) * 3600 + (tmLocal->tm_min - tmNow->tm_min) * 60;

    // Parse ISO 8601 timestamp and convert to local time
    console.println("ISO 8601 timestamp: " + timestampStr);
    struct tm tmTimestamp;
    strptime(timestampStr.c_str(), "%Y-%m-%dT%H:%M:%S%z", &tmTimestamp);
    time_t utcTime = mktime(&tmTimestamp);
    time_t localTime = utcTime + timeZone; // add timezone offset
    console.println("Local time: " + String(asctime(localtime(&localTime))));
  }
#endif

  console.log(F("Free heap: %d Max Free Block: %d"), ESP.getFreeHeap(), ESP.getMaxFreeBlockSize());

  #ifdef HTTP_OTA
    perform_HTTP_OTA_Update();
  #endif
}

#ifdef TIMER_INTERVAL_MILLIS

  void executeTimerCode() {
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

#ifdef TIMER_INTERVAL_MILLIS
  // https://www.norwegiancreations.com/2018/10/arduino-tutorial-avoiding-the-overflow-issue-when-using-millis-and-micros/
  if((unsigned long)(millis() - timer_now) > TIMER_INTERVAL_MILLIS){
      timer_now = millis();
      executeTimerCode();
  }
#else
  // put your main code here, to run repeatedly:
  
#endif

  watchdog.detach();

#ifdef DEEP_SLEEP_SECONDS
  // Enter DeepSleep
  console.println(F("Sleeping..."));
  ESP.deepSleep(DEEP_SLEEP_SECONDS * 1000000, WAKE_RF_DEFAULT);
  // Do nothing while we wait for sleep to overcome us
  while (true)
  {
  };
#endif
}