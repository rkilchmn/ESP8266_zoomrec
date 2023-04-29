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

// Optional functionality. Comment out defines to disable feature
#define WIFI_PORTAL                   // Enable WiFi config portal
#define WPS_CONFIG                    // press WPS bytton on wifi router
#define ARDUINO_OTA                   // Enable Arduino IDE OTA updates
// #define HTTP_OTA                      // Enable OTA updates from http server
#define LED_STATUS_FLASH              // Enable flashing LED status
// #define DEEP_SLEEP_SECONDS  300       // Define for sleep period between process repeats. No sleep if not defined
#define JSON_CONFIG_OTA                   // upload JSON config via OTA providing REST API
// #define GDB_DEBUG                    // enable debugging using GDB using serial 
#define NTP                             // enable NTP

#define FAST_CONNECTION_TIMEOUT 10000 // timeout for initial connection atempt 

#include <ESP8266WiFi.h>
#ifdef WIFI_PORTAL
  #include <DNSServer.h>            // Local DNS Server used for redirecting all requests to the configuration portal
  #include <ESP8266WebServer.h>     // Local WebServer used to serve the configuration portal
  #include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager WiFi Configuration Magic
  WiFiManager wifiManager;

  // const uint8 WIFI_PORTAL_TRIGGER_PIN = 4; // A low input on this pin will trigger the Wifi Manager Console at boot. Comment out to disable.
#else
  const char* WIFI_SSID = "SSID"
  const char* WIFI_PASSWORD = "password"
#endif

#ifdef ARDUINO_OTA
  /* Over The Air updates directly from Arduino IDE */
  #include <ESP8266mDNS.h>
  #include <WiFiUdp.h>
  #include <ArduinoOTA.h>

  #define ARDUINO_OTA_PORT      8266
  //#define ARDUINO_OTA_HOSTNAME  "esp8266"
  #define ARDUINO_OTA_PASSWD    "1csqBgpHchquXkzQlgl4"
#endif

#ifdef HTTP_OTA
  /* Over The Air automatic firmware update from a web server.  ESP8266 will contact the
   *  server on every boot and check for a firmware update.  If available, the update will
   *  be downloaded and installed.  Server can determine the appropriate firmware for this 
   *  device from any combination of HTTP_OTA_VERSION, MAC address, and firmware MD5 checksums.
   */
  #include <ESP8266HTTPClient.h>
  #include <ESP8266httpUpdate.h>

  #define HTTP_OTA_ADDRESS      F("192.168.#.###")   // Address of OTA update server
  #define HTTP_OTA_PATH         F("/esp8266-ota/update") // Path to update firmware
  #define HTTP_OTA_PORT         1888                     // Port of update server
                                                         // Name of firmware
  #define HTTP_OTA_VERSION      String(__FILE__).substring(String(__FILE__).lastIndexOf('/')+1) + ".generic" 
#endif

#ifdef JSON_CONFIG_OTA
  #include <FS.h>
  #include <ESP8266WebServer.h>  
  #include <ArduinoJson.h>

  #define MAX_JSON_SIZE 1024
  #define JSON_CONFIG_USERNAME "admin"
  #define JSON_CONFIG_PASSWD   "1csqBgpHchquXkzQlgl4"
  #define JSON_CONFIG_OTA_ROUTE "/config"
  #define JSON_CONFIG_OTA_PORT 80
  #define JSON_CONFIG_OTA_FILE "/config.json"
#endif

const uint8 STATUS_LED = LED_BUILTIN; // Built-in blue LED change if required e.g. pin 2
const char* SSID = (String("ESP") + String(ESP.getChipId())).c_str();

/* Watchdog to guard against the ESP8266 wasting battery power looking for 
 *  non-responsive wifi networks and servers. Expiry of the watchdog will trigger
 *  either a deep sleep cycle or a delayed reboot. The ESP8266 OS has another built-in 
 *  watchdog to protect against infinite loops and hangups in user code. 
 */
#include <Ticker.h>
Ticker watchdog;
const uint8 WATCHDOG_SETUP_SECONDS = 30;    // Setup should complete well within this time limit
const uint8 WATCHDOG_LOOP_SECONDS  = 20;    // Loop should complete well within this time limit

void timeout_cb() {
  // This sleep happened because of timeout. Do a restart after a sleep
  Serial.println(F("Watchdog timeout..."));

#ifdef DEEP_SLEEP_SECONDS
  // Enter DeepSleep so that we don't exhaust our batteries by countinuously trying to
  // connect to a network that isn't there. 
  ESP.deepSleep(DEEP_SLEEP_SECONDS * 1000, WAKE_RF_DEFAULT);
  // Do nothing while we wait for sleep to overcome us
  while(true){};

#else
  delay(1000);
  ESP.restart();
#endif
}

#ifdef LED_STATUS_FLASH

  #define LED_ON LOW   // Define the value to turn the LED on
  #define LED_OFF HIGH // Define the value to turn the LED off

  Ticker flasher;

  void flash() {
    digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
  }
#endif

#ifdef WIFI_PORTAL
  // Callback for entering config mode
  void configModeCallback (WiFiManager *myWiFiManager) {
    // Config mode has its own timeout
    watchdog.detach();

    #ifdef LED_STATUS_FLASH
      flasher.attach(0.2, flash);
    #endif
  }
#endif

#ifdef JSON_CONFIG_OTA 
  ESP8266WebServer server(JSON_CONFIG_OTA_PORT);

  void handleConfig() {
    if (!server.authenticate(JSON_CONFIG_USERNAME, JSON_CONFIG_PASSWD )) {
      return server.requestAuthentication();
    }

    if (server.method() == HTTP_POST) {
      // Only accept JSON content type
      if (server.hasHeader("Content-Type") && server.header("Content-Type") == "application/json") {
        // Parse JSON payload
        DynamicJsonDocument doc(MAX_JSON_SIZE);
        deserializeJson(doc, server.arg("plain"));

        // Store JSON payload in SPIFFS
        File configFile = SPIFFS.open( JSON_CONFIG_OTA_FILE, "w");
        if (configFile) {
          serializeJson(doc, configFile);
          configFile.close();
          server.send(200);
        } else {
          server.send(500, "text/plain", "Failed to open config file for writing");
        }
      } else {
        server.send(400, "text/plain", "Invalid content type: " + server.header("Content-Type"));
      }
    } else {
      server.send(405, "text/plain", "Method Not Allowed");
    }
  }
#endif

#ifdef GDB_DEBUG
  #include <GDBStub.h>
#endif

#ifdef JSON_CONFIG_OTA 
  bool retrieveJSON(DynamicJsonDocument& doc, String filename) {
    // read the JSON file from SPIFFS
    File file = SPIFFS.open(filename, "r");
    if (!file) {
      return false;
    }
    
    // parse the JSON file into the JSON object
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
      return false;
    }
    
    file.close();
    return true;
  }

  void printJSON(DynamicJsonDocument& doc) {
    // Print each value in the JSON document
    for (JsonPair pair : doc.as<JsonObject>()) {
      Serial.print(pair.key().c_str());
      Serial.print("=");
      serializeJson(pair.value(), Serial);
      Serial.println();
    }
  }
#endif

#ifdef NTP 
  // from https://werner.rothschopf.net/202011_arduino_esp8266_ntp_en.htm
  #define NTP_SERVER "pool.ntp.org"        
  // Timezone definition https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv   
  #define NTP_TIMEZONE "UTC0" 

  #include <coredecls.h> // optional callback to check on server

  void time_is_set(bool from_sntp /* <= this optional parameter can be used with ESP8266 Core 3.0.0*/) {
    Serial.print(F("time was sent! from_sntp=")); Serial.println(from_sntp);
    time_t now = time(nullptr);
    Serial.println(ctime(&now));
  }

  uint32_t sntp_startup_delay_MS_rfc_not_less_than_60000 ()
  {
    randomSeed(A0);
    return random(5000);
  }

  uint32_t sntp_update_delay_MS_rfc_not_less_than_15000 ()
  {
    return 12 * 60 * 60 * 1000UL; // 12 hours
  }
#endif


// Put any project specific initialisation here




void setup() {
  #ifdef GDB_DEBUG 
    gdbstub_init();
  #endif

  Serial.begin(115200);

  Serial.println(F("Booting"));

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
    Serial.print("WiFi credentials stored: ");
    Serial.println(WiFi.SSID());

    Serial.println("Connecting...");
    WiFi.begin(WiFi.SSID(), WiFi.psk());
    WiFi.waitForConnectResult(FAST_CONNECTION_TIMEOUT);
  }

#ifdef WPS_CONFIG
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Starting WPS configuration...");
    WiFi.beginWPSConfig();

    if (WiFi.SSID().length() > 0)
    {
      // Print the SSID and password
      Serial.print("WPS credentials received: ");
      Serial.println(WiFi.SSID());

      WiFi.begin(WiFi.SSID(), WiFi.psk());
      WiFi.persistent(true);
      WiFi.setAutoConnect(true);
      WiFi.setAutoReconnect(true);
      if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println(F("Connecting using WPS failed!"));
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
      if ( digitalRead(WIFI_PORTAL_TRIGGER_PIN) == LOW ) {
        watchdog.detach();
        if (!wifiManager.startConfigPortal(SSID, NULL)) {
          Serial.println(F("Config Portal Failed!"));
          timeout_cb();
        }
      } else {
    #endif
    
    wifiManager.setConfigPortalTimeout(180);
    wifiManager.setAPCallback(configModeCallback);
    if (!wifiManager.autoConnect()) {
      Serial.println(F("Connection Failed!"));
      timeout_cb();
    }

    #ifdef WIFI_PORTAL_TRIGGER_PIN
      }
    #endif
 
  #else
    // Save boot up time by not configuring them if they haven't changed
    if (WiFi.SSID() != WIFI_SSID) {
      Serial.println(F("Initialising Wifi..."));
      WiFi.mode(WIFI_STA);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      WiFi.persistent(true);
      WiFi.setAutoConnect(true);
      WiFi.setAutoReconnect(true);
    }
  #endif
  }

  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println(F("Connection Finally Failed!"));
    timeout_cb();
  }

  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());

#ifdef LED_STATUS_FLASH
  flasher.detach();
  digitalWrite(STATUS_LED, LED_OFF);
#endif

#ifdef HTTP_OTA
  // Check server for firmware updates
  Serial.print("Checking for firmware updates from server http://");
  Serial.print(HTTP_OTA_ADDRESS);
  Serial.print(":");
  Serial.print(HTTP_OTA_PORT);
  Serial.println(HTTP_OTA_PATH);

  WiFiClient client;
  #ifdef LED_STATUS_FLASH
    ESPhttpUpdate.setLedPin(STATUS_LED, LED_ON); // define level for LED on
  #endif
  switch(ESPhttpUpdate.update(client, HTTP_OTA_ADDRESS, HTTP_OTA_PORT, HTTP_OTA_PATH, HTTP_OTA_VERSION)) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP update failed: Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println(F("No updates"));
      break;

    case HTTP_UPDATE_OK:
      Serial.println(F("Update OK"));
      break;
    }
#endif

#ifdef ARDUINO_OTA
  // Arduino OTA Initalisation
  ArduinoOTA.setPort(ARDUINO_OTA_PORT);
  ArduinoOTA.setHostname(SSID);
  ArduinoOTA.setPassword(ARDUINO_OTA_PASSWD);
  ArduinoOTA.onStart([]() {
    watchdog.detach();
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
#endif

#ifdef JSON_CONFIG_OTA
   // Initialize SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("Failed to initialize SPIFFS");
  }

  // Handle HTTP POST request for config
  server.on( JSON_CONFIG_OTA_ROUTE, handleConfig);

  // list of headers to be parsed
  const char * headerkeys[] = {"Content-Type"} ;
  size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
  // ask server to parsed these headers
  server.collectHeaders(headerkeys, headerkeyssize );

  // Start server
  server.begin();
#endif


#ifdef JSON_CONFIG_OTA
  // show config
  DynamicJsonDocument config(MAX_JSON_SIZE);
  if (retrieveJSON(config, JSON_CONFIG_OTA_FILE)) 
    printJSON(config);
#endif

#ifdef NTP
  settimeofday_cb(time_is_set); // optional: callback if time was sent
  configTime( NTP_TIMEZONE, NTP_SERVER);
  // if (!config.isNull() && config.containsKey("timezone")) 
  //   configTime( config["timezone"], NTP_SERVER);
  // else
  //   configTime( NTP_TIMEZONE, NTP_SERVER);
#endif

// Put your initialisation code here




  

  Serial.println(F("Ready"));
  watchdog.detach();

}

void loop() {
  // Watchdog timer - resets if setup takes longer than allocated time
  watchdog.once(WATCHDOG_LOOP_SECONDS, &timeout_cb);

  // put your main code here, to run repeatedly:
  delay(10000);

  watchdog.detach();
  
#ifdef ARDUINO_OTA
  // Handle any OTA upgrade
  ArduinoOTA.handle();
#endif

#ifdef ARDUINO_OTA
  // Handle HTTP requests
  server.handleClient();
#endif

#ifdef DEEP_SLEEP_SECONDS
  // Enter DeepSleep
  Serial.println(F("Sleeping..."));
  ESP.deepSleep(DEEP_SLEEP_SECONDS * 1000000, WAKE_RF_DEFAULT);
  // Do nothing while we wait for sleep to overcome us
  while(true){};
#endif
  
}