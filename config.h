#ifndef CONFIG_H
#define CONFIG_H

#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <FS.h>

class Config
{
public:
    static const size_t JSON_CONFIG_MAXSIZE = 4096;
    const char *JSON_CONFIG_OTA_FILE = "/config.json";
    const char *JSON_CONFIG_USERNAME = "admin";
    const char *JSON_CONFIG_PASSWD = "myadminpw";
    static const int JSON_CONFIG_OTA_PORT = 80;

    Config();
    bool retrieveJSON();
    int hasConfig(const char *configKey);
    int useConfig(const char *configKey, int defaultValue);
    const char *useConfig(const char *configKey, const char *defaultValue);

    // for config http server
    void setup_JSON_CONFIG_OTA();
    void handleConfig();

private:
    DynamicJsonDocument doc;
    ESP8266WebServer server;
};

#endif // CONFIG_H
