#ifndef FEATURES_H
#define FEATURES_H

// Optional functionality. Comment out defines to disable feature
#define WIFI_PORTAL                   // Enable WiFi config portal
#define WPS_CONFIG                    // press WPS bytton on wifi pathr
#define ARDUINO_OTA                   // Enable Arduino IDE OTA updates: caused segfault after ca 30 min:
// #define HTTP_OTA                      // Enable OTA updates from http server
#define LED_STATUS_FLASH              // Enable flashing LED status
#define DEEP_SLEEP_SECONDS 20         // Define for sleep timer_interval between process repeats. No sleep if not defined
#define DEEP_SLEEP_STARTUP_SECONDS 40 // do not fall into deep sleep after normal startup, to allow for OTA updates
// #define TIMER_INTERVAL_MILLIS 20000    // periodically execute code using non-blocking timer instead delay()
#define JSON_CONFIG_OTA               // upload JSON config via OTA providing REST API
#define GDB_DEBUG                     // enable debugging using GDB using serial
// #define CONSOLE_TELNET                // console output can be accessed by telnet server on ESP
#define CONSOLE_HTTP               // console output sent to a HTTP server app to view
#define USE_NTP                       // connect to NTP server to retrieve time

// important for reliable deep sleep wake up and reset via UART for flashing
// use a BAT43 Schottky Diode: RST (Anode +) ---D|--- (Cathode -) D0
// https://github.com/universam1/iSpindel/issues/59


#endif // FEATURES_H