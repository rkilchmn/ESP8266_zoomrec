#include "config.h"

Config::Config() : doc(0), server(JSON_CONFIG_OTA_PORT); {
    if (SPIFFS.begin()) 
        retrieveJSON();
}

bool Config::retrieveJSON() {
  File file = SPIFFS.open(JSON_CONFIG_OTA_FILE, "r");
  if (!file) {
    return false;
  }

  doc = DynamicJsonDocument(JSON_CONFIG_MAXSIZE);
  DeserializationError error = deserializeJson(doc, file);
  doc.shrinkToFit();
  file.close();

  return !error;
}

int Config::hasConfig(const char *configKey) {
  if (!doc.isNull() && doc.containsKey(configKey))
    return true;
  else
    return false;
}

int Config::useConfig(const char *configKey, int defaultValue) {
  if (!doc.isNull() && doc.containsKey(configKey))
    return doc[configKey].as<int>();
  else
    return defaultValue;
}

const char *Config::useConfig(const char *configKey, const char *defaultValue) {
  if (!doc.isNull() && doc.containsKey(configKey))
    return doc[configKey];
  else
    return defaultValue;
}

void Config::handleConfig()
{
  if (!server.authenticate(useConfig("json_config_ota_username", JSON_CONFIG_USERNAME),
                           useConfig("json_config_ota_password", JSON_CONFIG_PASSWD)))
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
        // re-read from filesystem & output
        retrieveJSON(config, JSON_CONFIG_OTA_FILE); // refresh config
        console.log(Console::INFO, F("OTA Config update received from IP: %s"), server.client().remoteIP().toString().c_str());
        serializeJsonPretty(config, console);
        console.println();
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

void Config::setup_JSON_CONFIG_OTA()
{
  // Handle HTTP POST request for config
  server.on(useConfig("json_config_ota_path", JSON_CONFIG_OTA_PATH), handleConfig);

  // list of headers to be parsed
  const char *headerkeys[] = {"Content-Type"};
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char *);
  // ask server to parsed these headers
  server.collectHeaders(headerkeys, headerkeyssize);

  // Start server
  int port = config["json_config_ota_port"].as<int>();
  port = (port) ? port : JSON_CONFIG_OTA_PORT; // fallback to default
  console.log(Console::INFO, F("Starting Config OTA Server on port: %d"), port);
  
  if (port)
    server.begin(port);
  else
    server.begin();
}
