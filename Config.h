#ifndef CONFIG_H
#define CONFIG_H

#include <ArduinoJson.h> // git clone https://github.com/bblanchon/ArduinoJson.git
#include <ESP8266WebServer.h>

#include "Console.h"

class Config
{
public:
    Config();

    int exists(const char *configKey);
    int get(const char *configKey, int defaultValue);
    const char *get(const char *configKey, const char *defaultValue = "");
    void print(Console* console);
    // for config http server
    void setupOTAServer(Console* console);
    void handleOTAServerClient();

protected:
    static const size_t JSON_CONFIG_MAXSIZE = 4096;
    const char *JSON_CONFIG_OTA_FILE = "/config.json";
    const char *JSON_CONFIG_USERNAME = "admin";
    const char *JSON_CONFIG_PASSWD = "myadminpw";
    const char *JSON_CONFIG_OTA_PATH = "/config";
    static const int JSON_CONFIG_OTA_PORT = 80;

    DynamicJsonDocument doc;
    ESP8266WebServer server;
    Console *refConsole = nullptr;

    void handleOTAServerRequest();
    bool retrieveJSON();
};

#endif // CONFIG_H
