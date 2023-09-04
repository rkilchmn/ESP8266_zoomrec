#include <Arduino.h>
#include <UrlEncode.h> // git clone https://github.com/plageoj/urlencode.git
#include <ArduinoJson.h> // git clone https://github.com/bblanchon/ArduinoJson.git

#include "BaseApp.h"
#include "HTTPRest.h"
#include "JSONRtcMemory.h"

class ZoomrecApp : public BaseApp
{
  public:
    ZoomrecApp() : statusDoc(RTC_MEMORY_SIZE * 2) {
      // get status
      if (!JSONRtcMemory::load(statusDoc, RTC_MEMORY_SIZE)) {
        statusDoc["on"] = 0;
        statusDoc.shrinkToFit();
      }
    };

private:
  const int inputPinResetSwitch = D1;
  const int outputPinPowerButton = D2;
  
  const size_t RTC_MEMORY_SIZE = 512; 
  DynamicJsonDocument statusDoc;

  bool checked = false;
  
  void setFirmwareVersion() {
    FIRMWARE_VERSION = String(__FILE__) + "-" + String(__DATE__) + "-" + String(__TIME__);
  }

void printStatus() {
  serializeJsonPretty(statusDoc, console);
  console.println();
}

  
  void AppSetup()
  {
    // Put your initialisation code here
    pinMode(inputPinResetSwitch, INPUT_PULLUP);
    pinMode(outputPinPowerButton, OUTPUT);

    printStatus();
  }

  void AppLoop()
  {
    if (ntp_set) {
      if  (!checked) { // check once
        checkUpdateStatus();
        checked = true; 
        preventDeepSleep = false;
      }
    }
    else {
      preventDeepSleep = true;
    }
  }

  time_t convertISO8601ToUnixTime(const char *isoTimestamp)
  {
    struct tm timeinfo;
    memset(&timeinfo, 0, sizeof(timeinfo));

    // Parse the ISO8601 timestamp
    sscanf(isoTimestamp, "%4d-%2d-%2dT%2d:%2d:%2d",
           &timeinfo.tm_year, &timeinfo.tm_mon, &timeinfo.tm_mday,
           &timeinfo.tm_hour, &timeinfo.tm_min, &timeinfo.tm_sec,
           &timeinfo.tm_hour, &timeinfo.tm_min);

    // Adjust year and month for struct tm format
    timeinfo.tm_year -= 1900;
    timeinfo.tm_mon -= 1;

    // Convert to Unix time
    time_t unixTime = mktime(&timeinfo);

    return unixTime;
  }

  const char *unixTimestampToISO8601(time_t unixTimestamp, char *iso8601Time, size_t bufferSize)
  {
    struct tm *tmStruct = localtime(&unixTimestamp);
    snprintf(iso8601Time, bufferSize, "%04d-%02d-%02dT%02d:%02d:%02d",
             tmStruct->tm_year + 1900, tmStruct->tm_mon + 1, tmStruct->tm_mday,
             tmStruct->tm_hour, tmStruct->tm_min, tmStruct->tm_sec);
    return iso8601Time;
  }

  bool isPCRunning()
  {
    return digitalRead(inputPinResetSwitch) == HIGH;
  }

  void togglePowerButton()
  {
    digitalWrite(outputPinPowerButton, HIGH);
    delay(150);
    digitalWrite(outputPinPowerButton, LOW);
  }

  void startPC()
  {
    bool onValue = isPCRunning();
    statusDoc["on"] = onValue;

    if (!onValue) {
      togglePowerButton();
      printStatus();
    }

    // JSONRtcMemory::save(statusDoc, RTC_MEMORY_SIZE);
  }

  void shutDownPC()
  {
    if (isPCRunning()) {
      int onValue = statusDoc["on"].as<int>();

      if (!onValue) { // PC was off, so it switched off
        togglePowerButton();
        // JSONRtcMemory::save(statusDoc, RTC_MEMORY_SIZE);
        printStatus();
      }
        
    }
  }

  void checkUpdateStatus()
  {

    console.log(Console::DEBUG, F("PC status: %s"), isPCRunning() ? "true" : "false");

    // test http client
    if (!config.exists("http_api_base_url") || (!config.exists("timezone")))
    {
      console.log(Console::CRITICAL, F("Config 'http_api_base_url' or 'timezone' missing."));
    }
    else
    {
      // API route
      char path[100];
      snprintf(path, sizeof(path), "/event/next?astimezone=%s&leadinsecs=%d&leadoutsecs=%d",
               urlEncode(config.get("timezone", "")).c_str(), config.get("leadin_secs", 60), config.get("leadout_secs", 60));

      DynamicJsonDocument response = performHttpsRequest(console, "GET", config.get("http_api_base_url", ""), path,
                                                         config.get("http_api_username", ""), config.get("http_api_password", ""), "");

      bool eventOngoing = false;
      if (response.isNull())
      {
        console.log(Console::DEBUG, F("response for /event/next is empty"));
      }
      else
      {
        serializeJsonPretty(response, console);
        console.println();

        // extract
        char startStr[30];
        char endStr[30];

        strcpy(startStr, response["start_astimezone"]);
        strcpy(endStr, response["end_astimezone"]);

        if ((startStr == nullptr || *startStr == '\0') ||
            (endStr == nullptr || *endStr == '\0'))
        {
          console.log(Console::DEBUG, F("response does not contain start_astimezone or end_astimezone"));
        }
        else
        {
          // Convert ISO 8601 timestamps to time_t (Unix timestamp)
          time_t startTime = convertISO8601ToUnixTime(startStr);
          time_t endTime = convertISO8601ToUnixTime(endStr);

          // Get current UTC time
          time_t currentTime = time(nullptr);

          // Check if the event has started and has not yet ended
          if ((currentTime >= startTime) && (currentTime <= endTime))
            eventOngoing = true;
        }
      }

      if (eventOngoing)
      {
        console.log(Console::DEBUG, F("Event ongoing..."));
        if (!isPCRunning())
        {
          startPC();
          console.log(Console::DEBUG, F("Starting PC..."));
        }
      }
      else
      {
        console.log(Console::DEBUG, F("No event ongoing..."));
        if (isPCRunning())
        {
          shutDownPC();
          console.log(Console::DEBUG, F("Shutting down PC..."));
        }
      }
    }

    console.log(Console::DEBUG, F("Free heap: %d Max Free Block: %d"), ESP.getFreeHeap(), ESP.getMaxFreeBlockSize());
  }
};

ZoomrecApp app;

void setup()
{
  app.setup();
}

void loop()
{
  app.loop();
}