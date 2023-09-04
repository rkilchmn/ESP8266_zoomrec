#ifndef FEATURES_H
#define FEATURES_H

// Optional functionality. Comment out defines to disable feature
#define WIFI_PORTAL                   // Enable WiFi config portal
#define WPS_CONFIG                    // press WPS bytton on wifi pathr
#define ARDUINO_OTA                   // Enable Arduino IDE OTA updates: caused segfault after ca 30 min:
#define HTTP_OTA                      // Enable OTA updates from http server
#define LED_STATUS_FLASH              // Enable flashing LED status
#define DEEP_SLEEP_SECONDS 60         // Define for sleep timer_interval between process repeats. No sleep if not defined
#define DEEP_SLEEP_STARTUP_SECONDS 60 // do not fall into deep sleep after normal startup, to allow for OTA updates
// #define TIMER_INTERVAL_MILLIS 20000    // periodically execute code using non-blocking timer instead delay()
#define JSON_CONFIG_OTA               // upload JSON config via OTA providing REST API
#define GDB_DEBUG                     // enable debugging using GDB using serial
#define USE_NTP                       // enable NTP
#define TELNET                        // use telnet

#endif // FEATURES_H