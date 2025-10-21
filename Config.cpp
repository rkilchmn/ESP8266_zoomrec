#include "Config.h"

#include "features.h"
// library
#include <LittleFS.h>
// project
#include "Console.h"

Config::Config() : configJsonDoc(JSON_CONFIG_MAXSIZE), server(JSON_CONFIG_OTA_PORT)
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

  DeserializationError error = deserializeJson(configJsonDoc, file); // implicitly calls configJsonDoc.clear();
  configJsonDoc.shrinkToFit();
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
  if (!configJsonDoc.isNull() && configJsonDoc.containsKey(configKey))
    return true;
  else
    return false;
}

int Config::get(const char *configKey, int defaultValue)
{
  if (!configJsonDoc.isNull() && configJsonDoc.containsKey(configKey))
    return configJsonDoc[configKey].as<int>();
  else
    return defaultValue;
}

const char *Config::get(const char *configKey, const char *defaultValue)
{
  if (!configJsonDoc.isNull() && configJsonDoc.containsKey(configKey))
    return configJsonDoc[configKey];
  else
    return defaultValue;
}

#ifdef JSON_CONFIG_OTA
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
#endif // JSON_CONFIG_OTA

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
  if ((configJsonDoc != nullptr) && (console != nullptr))
  {
    serializeJsonPretty(configJsonDoc, *console);
    console->println();
  }
}

#ifdef HTTP_CONFIG
bool Config::performHttpConfigUpdate(const String& firmwareVersion, Console* console) {
  String http_config_url = get("http_config_url", HTTP_CONFIG_URL);
  if (http_config_url.isEmpty()) {
    if (console) {
      console->log(Console::WARNING, F("No HTTP config URL configured"));
    }
    return false;
  }
  
  String http_config_username = get("http_config_username", HTTP_CONFIG_USERNAME);
  String http_config_password = get("http_config_password", HTTP_CONFIG_PASSWORD);

  if (console) {
    console->log(Console::INFO, F("Checking for config update via HTTP from %s"), http_config_url.c_str());
  }

  // Allocate JSON documents for request and response
  DynamicJsonDocument requestHeader(256);  // Headers with version info and last_updated
  DynamicJsonDocument requestBody(0);      // Empty body for GET request
  
  // Add version header
  requestHeader["x-ESP8266-version"] = firmwareVersion;
  requestHeader["x-ESP8266-config-version"] = get("version", "");

  // To free current memory and recreate with full capacity
  configJsonDoc = DynamicJsonDocument(JSON_CONFIG_MAXSIZE);

  // Use managed client matching the URL scheme
  int httpCode = JSONAPIClient::performRequest(
    *ManageWifiClient::getClient(http_config_url.c_str()),
    JSONAPIClient::HTTP_METHOD_GET,
    http_config_url.c_str(),
    "",
    requestHeader,
    requestBody,
    configJsonDoc,
    http_config_username.c_str(),
    http_config_password.c_str()
  );

  bool result = false;
  bool saved = false;

  switch (httpCode) {
    case HTTP_CODE_OK:
      // Save the configuration
      if (saveConfig(configJsonDoc)) {
        result = true;
        saved = true;
        if (console) {
          console->log(Console::INFO, F("Successfully updated config via HTTP from %s"), http_config_url.c_str());
        }
      } else {
        if (console) {
          console->log(Console::ERROR, F("Failed to save config"));
        }
      }
      break;
      
    case HTTP_CODE_NOT_MODIFIED:
    case HTTP_CODE_NO_CONTENT:
      if (console) {
        console->log(Console::INFO, F("No Config Update available via HTTP"));
      }
      result = true;
      break;
      
    default:
      if (console) {
        console->log(Console::ERROR, F("HTTP request %s failed with code: %d"), http_config_url.c_str(), httpCode);
        if (configJsonDoc.containsKey("message")) {
          console->log(Console::ERROR, F("message: %s"), configJsonDoc["message"].as<String>().c_str());
        }
      }
  }

  // reinstate config if not successfully received and saved
  if (!saved) {
    retrieveJSON();
  }

  return result;
}

#endif // HTTP_CONFIG
