#include "Config.h"
// library
#include <LittleFS.h>
// project
#include "Console.h"

Config::Config() : doc(0), server(JSON_CONFIG_OTA_PORT)
{
  if (LittleFS.begin())
    retrieveJSON();
}

bool Config::retrieveJSON()
{
  File file = LittleFS.open(JSON_CONFIG_OTA_FILE, "r");
  if (!file)
  {
    return false;
  }

  doc = DynamicJsonDocument(JSON_CONFIG_MAXSIZE);
  DeserializationError error = deserializeJson(doc, file);
  doc.shrinkToFit();
  file.close();

  return !error;
}

time_t Config::getConfigTimestamp()
{
  if (!LittleFS.exists(JSON_CONFIG_OTA_FILE)) {
    return 0;  // Return 0 if file doesn't exist
  }
  
  File file = LittleFS.open(JSON_CONFIG_OTA_FILE, "r");
  if (!file) {
    return 0;  // Return 0 if file can't be opened
  }
  
  time_t lastModified = file.getLastWrite();
  size_t fileSize = file.size();
  file.close();
  
  return lastModified;
}

int Config::exists(const char *configKey)
{
  if (!doc.isNull() && doc.containsKey(configKey))
    return true;
  else
    return false;
}

int Config::get(const char *configKey, int defaultValue)
{
  if (!doc.isNull() && doc.containsKey(configKey))
    return doc[configKey].as<int>();
  else
    return defaultValue;
}

const char *Config::get(const char *configKey, const char *defaultValue)
{
  if (!doc.isNull() && doc.containsKey(configKey))
    return doc[configKey];
  else
    return defaultValue;
}

void Config::handleOTAServerRequest()
{
  if (!server.authenticate(get("json_config_ota_username", JSON_CONFIG_USERNAME),
                           get("json_config_ota_password", JSON_CONFIG_PASSWD)))
  {
    return server.requestAuthentication();
  }

  if (server.method() == HTTP_POST)
  {
    // Only accept JSON content type
    if (server.hasHeader("Content-Type") && server.header("Content-Type") == "application/json")
    {
      // Parse JSON payload
      DynamicJsonDocument configDoc(JSON_CONFIG_MAXSIZE);
      DeserializationError error = deserializeJson( configDoc, server.arg("plain"));
      if (error) {
        server.send(400, "text/plain", error.c_str());
      }
      else {        
        if (saveConfig(configDoc)) {
          // Log the incoming request
          if (refConsole != nullptr) {
            refConsole->log(Console::INFO, F("OTA Config update received from IP: %s"), server.client().remoteIP().toString().c_str());
          }
          
          server.send(200);
        } else {
          server.send(400, "text/plain", "Failed to save configuration");
        }
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

void Config::setupOtaServer(Console *console)
{
  refConsole = console; // server callback handler handleOTAServerRequest uses console
  // Handle HTTP POST request for config
  server.on(get("json_config_ota_path", JSON_CONFIG_OTA_PATH), std::bind(&Config::handleOTAServerRequest, this));

  // list of headers to be parsed
  const char *headerkeys[] = {"Content-Type"};
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char *);
  // ask server to parsed these headers
  server.collectHeaders(headerkeys, headerkeyssize);

  // Start server
  int port = get("json_config_ota_port", JSON_CONFIG_OTA_PORT);
  server.begin(port);
  if (refConsole != nullptr) {
    refConsole->log(Console::INFO, F("Config OTA Server started on port: %d"), port);
  }
}

void Config::handleOTAServerClient() {
  server.handleClient();
}

bool Config::saveConfig(DynamicJsonDocument& configDoc)
{
  configDoc.shrinkToFit();

  // Store JSON payload in LittleFS
  File configFile = LittleFS.open(JSON_CONFIG_OTA_FILE, "w");
  if (!configFile)
  {
    return false;
  }

  int size = serializeJson(configDoc, configFile);
  configFile.close();
  
  if (size <= 0) {
    return false;
  }

  // Re-read from filesystem
  if (!retrieveJSON()) {
    return false;
  }

  return true;
}

void Config::print(Console* console)
{
  if ((doc != nullptr) && (console != nullptr))
  {
    serializeJsonPretty(doc, *console);
    console->println();
  }
}
