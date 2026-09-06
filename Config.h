#ifndef CONFIG_H
#define CONFIG_H

#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

#include "features.h"
#include "Console.h"
#include "JSONAPIClient.h"
#include "ManageWifiClient.h"

class Config
{
public:
    Config();

    int exists(const char *configKey);
    int get(const char *configKey, int defaultValue);
    const char *get(const char *configKey, const char *defaultValue = "");
    void print(Console* console, DynamicJsonDocument* config = nullptr);
#ifdef JSON_CONFIG_OTA
    void setupOtaServer(Console* console);
    void handleOTAServerClient();
#endif // JSON_CONFIG_OTA
    bool saveConfig(const String& json);
    time_t getConfigTimestamp();
#ifdef HTTP_CONFIG
    bool performHttpConfigUpdate(const String& firmwareVersion, Console* console);
#endif // HTTP_CONFIG
    static const size_t JSON_CONFIG_MAXSIZE = 2048;

protected:
    const char *JSON_CONFIG_OTA_FILE = "/config.json";
    const char *JSON_CONFIG_OTA_TMP_FILE = "/config.json.tmp";
    const char *JSON_CONFIG_OTA_BAK_FILE = "/config.json.bak";
    const char *JSON_CONFIG_USERNAME = "user";
    const char *JSON_CONFIG_PASSWD = "myuserpw";
    const char *JSON_CONFIG_OTA_PATH = "/config";
    static const int JSON_CONFIG_OTA_PORT = 8080;
    
#ifdef HTTP_CONFIG
    // HTTP Config Update settings
    const char *HTTP_CONFIG_URL = "http://192.168.0.1:8080/config";
    const char *HTTP_CONFIG_USERNAME = "user";
    const char *HTTP_CONFIG_PASSWORD = "myuserpw";
#endif // HTTP_CONFIG

    DynamicJsonDocument configJsonDoc;
    Console *refConsole = nullptr;

    template<typename... Args>
    void log(Console::LogLevel level, const __FlashStringHelper *format, Args... args) {
      if (refConsole != nullptr) {
        refConsole->log(level, format, args...);
      }
    }

#ifdef JSON_CONFIG_OTA
    ESP8266WebServer server;
    void handleOTAServerRequest();
#endif // JSON_CONFIG_OTA
    bool readConfig(const char *path = nullptr);
    void listFiles();
};

#endif // CONFIG_H
