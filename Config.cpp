#include "Config.h"

#include "features.h"
// library
#include <LittleFS.h>
// project
#include "Console.h"

Config::Config() : configJsonDoc(JSON_CONFIG_MAXSIZE), server(JSON_CONFIG_OTA_PORT)
{
  if (LittleFS.begin())
    readConfig();
}

bool Config::readConfig(const char *path)
{
  if (path == nullptr) {
    path = JSON_CONFIG_OTA_FILE;
  }

  File file = LittleFS.open(path, "r");
  if (!file)
  {
    log(Console::ERROR, F("readConfig: failed to open file %s"), path);
    return false;
  }

  configJsonDoc = DynamicJsonDocument(JSON_CONFIG_MAXSIZE);
  DeserializationError error = deserializeJson(configJsonDoc, file); // implicitly calls configJsonDoc.clear();
  file.close();

  if (error) {
    log(Console::ERROR, F("readConfig: deserialization error in %s: %s"), path, error.c_str());
    return false;
  }

  configJsonDoc.shrinkToFit();
  log(Console::DEBUG, F("configJsonDoc memory usage: %d of max. %d bytes"), configJsonDoc.memoryUsage(), JSON_CONFIG_MAXSIZE);

  return true;
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
  int result_code = 200;
  String message;

  if (!server.authenticate(get("json_config_ota_username", JSON_CONFIG_USERNAME),
                           get("json_config_ota_password", JSON_CONFIG_PASSWD)))
  {
    return server.requestAuthentication();
  }

  String clientIP = server.client().remoteIP().toString();

  if (server.method() == HTTP_POST)
  {
    // Only accept JSON content type
    if (server.hasHeader("Content-Type") && server.header("Content-Type") == "application/json")
    {
      // Parse JSON payload
      // To free current memory and recreate with full capacity
      configJsonDoc = DynamicJsonDocument(JSON_CONFIG_MAXSIZE);
      DeserializationError error = deserializeJson( configJsonDoc, server.arg("plain"));
      if (error) {
        result_code = HTTP_CODE_INTERNAL_SERVER_ERROR;
        message = F("De-serialization error: ");
        message += error.c_str();
      }
      else {
        if (saveConfig(server.arg("plain"))) {
          result_code = HTTP_CODE_OK;
          message = F("Configuration successfully updated");
        } else {
          result_code = HTTP_CODE_INTERNAL_SERVER_ERROR;
          message = F("Failed to save configuration");
        }
      }
      if (result_code != HTTP_CODE_OK) {
        // failed -> reinstate old config
        readConfig();  
      }
    }
    else
    {
      result_code = HTTP_CODE_BAD_REQUEST;
      message = F("Content type 'application/json' expected, received: ") + server.header("Content-Type");
    }
  }
  else
  {
    result_code = HTTP_CODE_METHOD_NOT_ALLOWED;
    message = F("Method Not Allowed");
  }

  log(Console::INFO, F("OTA Config Update from IP: %s Result: %d - %s"), clientIP.c_str(), result_code, message.c_str());
  server.send(result_code, "text/plain", message);
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
  log(Console::INFO, F("Config OTA Server started on port: %d"), port);
}

void Config::handleOTAServerClient() {
  server.handleClient();
}
#endif // JSON_CONFIG_OTA

bool Config::saveConfig(const String& json)
{
  log(Console::DEBUG, F("saveConfig: json length=%d"), (int)json.length());

  FSInfo fs_info;
  LittleFS.info(fs_info);
  log(Console::DEBUG, F("LittleFS: total=%d, used=%d, block=%d"), (int)fs_info.totalBytes, (int)fs_info.usedBytes, (int)fs_info.blockSize);

  // 1. Write the raw JSON directly to a temporary file.
  if (LittleFS.exists(JSON_CONFIG_OTA_TMP_FILE)) {
    LittleFS.remove(JSON_CONFIG_OTA_TMP_FILE);
  }

  File tmpFile = LittleFS.open(JSON_CONFIG_OTA_TMP_FILE, "w");
  if (!tmpFile)
  {
    log(Console::ERROR, F("saveConfig: failed to open temp file for writing"));
    return false;
  }

  const uint8_t *data = reinterpret_cast<const uint8_t *>(json.c_str());
  size_t expected = json.length();

  size_t written = tmpFile.write(data, expected);

  if (written != expected) {
    log(Console::ERROR,
          F("saveConfig: write() wrote %d bytes, expected %d"),
          (int)written,
          (int)expected
      );
    tmpFile.close();
    LittleFS.remove(JSON_CONFIG_OTA_TMP_FILE);
    return false;
  }

  tmpFile.flush();
  tmpFile.close();

  // 2. Verify the exact number of bytes were written to the temporary file.
  File tmpCheck = LittleFS.open(JSON_CONFIG_OTA_TMP_FILE, "r");
  if (!tmpCheck || tmpCheck.size() != json.length()) {
    int actualSize = tmpCheck ? (int)tmpCheck.size() : -1;
    if (tmpCheck) {
      tmpCheck.close();
    }
    log(Console::ERROR, F("saveConfig: temp file size %d, expected %d bytes"), actualSize, (int)json.length());
    LittleFS.remove(JSON_CONFIG_OTA_TMP_FILE);
    return false;
  }
  tmpCheck.close();

  // 3. Verify the temporary file is complete and valid before touching the original.
  if (!readConfig(JSON_CONFIG_OTA_TMP_FILE)) {
    log(Console::ERROR, F("saveConfig: temp file failed validation"));
    LittleFS.remove(JSON_CONFIG_OTA_TMP_FILE);
    return false;
  }

  // 4. Rotate: move the original aside as a backup, then promote the temp file.
  bool hadOriginal = LittleFS.exists(JSON_CONFIG_OTA_FILE);

  if (hadOriginal) {
    if (LittleFS.exists(JSON_CONFIG_OTA_BAK_FILE)) {
      LittleFS.remove(JSON_CONFIG_OTA_BAK_FILE);
    }
    if (!LittleFS.rename(JSON_CONFIG_OTA_FILE, JSON_CONFIG_OTA_BAK_FILE)) {
      log(Console::ERROR, F("saveConfig: failed to rename original to backup"));
      LittleFS.remove(JSON_CONFIG_OTA_TMP_FILE);
      return false;
    }
  }

  if (!LittleFS.rename(JSON_CONFIG_OTA_TMP_FILE, JSON_CONFIG_OTA_FILE)) {
      log(Console::ERROR, F("saveConfig: failed to promote temp file to original"));
    if (hadOriginal) {
      if (!LittleFS.rename(JSON_CONFIG_OTA_BAK_FILE, JSON_CONFIG_OTA_FILE)) {
        log(Console::ERROR, F("saveConfig: failed to restore original from backup"));
      }
    }
    LittleFS.remove(JSON_CONFIG_OTA_TMP_FILE);
    return false;
  }

  // 5. Success: remove the backup.
  if (hadOriginal && LittleFS.exists(JSON_CONFIG_OTA_BAK_FILE)) {
    LittleFS.remove(JSON_CONFIG_OTA_BAK_FILE);
  }

  return true;
}

void Config::print(Console* console, DynamicJsonDocument* config)
{
  JsonDocument& doc = config ? *config : configJsonDoc;
  if (!doc.isNull() && (console != nullptr))
  {
    serializeJsonPretty(doc, *console);
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
    case HTTP_CODE_OK: {
      // Serialize the received config back to JSON and save it byte-for-byte.
      String json;
      serializeJson(configJsonDoc, json);
      if (saveConfig(json)) {
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
    }
      
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
    readConfig();
  }

  return result;
}

#endif // HTTP_CONFIG
