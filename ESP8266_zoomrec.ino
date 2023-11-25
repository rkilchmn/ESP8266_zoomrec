#include <Arduino.h>
#include <UrlEncode.h>   // git clone https://github.com/plageoj/urlencode.git
#include <ArduinoJson.h> // git clone https://github.com/bblanchon/ArduinoJson.git

#include "BaseApp.h"
#include "JSONAPIClient.h"

class ZoomrecApp : public BaseApp
{
public:
  ZoomrecApp()
  {
    // register deep sleep state variables
    deepSleepState.registerVar(&changedPowerState);
  }

private:
  const int INPUTPINRESETSWITCH = D1;
  const int OUTPUTPINPOWERBUTTON = D2;

  const int PC_ON = 1;
  const int PC_OFF = 0;

  // define reset/deepSleep-safe RTC memory state variables
  int changedPowerState;

  bool checked = false;

  void AppFirmwareVersion()
  {
    FIRMWARE_VERSION = String(__FILE__) + "-" + String(__DATE__) + "-" + String(__TIME__);
  }

  void AppDeepSleepStateInit()
  {
    changedPowerState = false;
    console.log(Console::DEBUG, F("DeepSleepState cold boot - initializing: changedPowerState=%d"), changedPowerState);
  }

  void AppSetup()
  {
    // Put your initialisation code here
    pinMode(INPUTPINRESETSWITCH, INPUT_PULLUP);
    pinMode(OUTPUTPINPOWERBUTTON, OUTPUT);

    console.log(Console::DEBUG, F("DeepSleepState: changedPowerState=%d"), changedPowerState);
  }

  void AppLoop()
  {
    if (ntp_set)
    {
      if (!checked)
      { // check once
        // callLogServer();
        checkUpdateStatus();
        checked = true;
        preventDeepSleep = false;
      }
    }
    else
    {
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

  bool getPowerState()
  {
    return digitalRead(INPUTPINRESETSWITCH) == HIGH ? PC_ON : PC_OFF;
  }

  void togglePowerButton()
  {
    digitalWrite(OUTPUTPINPOWERBUTTON, HIGH);
    delay(150);
    digitalWrite(OUTPUTPINPOWERBUTTON, LOW);
  }

  void startPC()
  {
    if (getPowerState() == PC_OFF) {
      togglePowerButton();
    }
  }

  void shutDownPC()
  {
    if (getPowerState() == PC_ON)
    {
      if (changedPowerState) // did we change the power state?
      { 
        // yes, we are also shutting down PC
        togglePowerButton();
      }
    }
  }

  void callLogServer() {

    console.log(Console::DEBUG, F("Call Log Server "));

    DynamicJsonDocument request  = DynamicJsonDocument(200);

    request["id"] = "test_log";
    request["content"] = "mytestlog content\n";
    request.shrinkToFit();

    DynamicJsonDocument response = JSONAPIClient::performRequest( 
      JSONAPIClient::HTTP_METHOD_POST, "http://192.168.0.237:8080","/log", request,
      "admin","myadminpw", "");      

    serializeJsonPretty(response, console);
    console.println();                              
  }


  void checkUpdateStatus()
  {
    console.log(Console::DEBUG, F("PC powerState: %s"), getPowerState() == PC_ON ? "PC_ON" : "PC_OFF");

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

      DynamicJsonDocument response = JSONAPIClient::performRequest(
        JSONAPIClient::HTTP_METHOD_GET, config.get("http_api_base_url", ""), path, DynamicJsonDocument(0),
        config.get("http_api_username", ""), config.get("http_api_password", ""), ""
      );

      serializeJsonPretty(response, Serial);

      bool eventOngoing = false;

      if (response["code"].as<int>() == HTTP_CODE_OK) {    
        if (response["body"].isNull())
        {
          console.log(Console::DEBUG, F("response for /event/next is empty"));
        }
        else
        {
          // extract
          char startStr[30];
          char endStr[30];

          strcpy(startStr, response["body"]["start_astimezone"]);
          strcpy(endStr, response["body"]["end_astimezone"]);

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
      }
      else {
        // something failed
        serializeJsonPretty(response, console);
        console.println();
      }

      if (eventOngoing)
      {
        console.log(Console::DEBUG, F("Event ongoing..."));
        if (getPowerState() == PC_OFF)
        {
          console.log(Console::INFO, F("Starting PC..."));
          startPC();
          changedPowerState = true; // we changed power state
        }
      }
      else
      {
        console.log(Console::DEBUG, F("No event ongoing..."));
        if (getPowerState() == PC_ON)
        {
          if (changedPowerState) { // did we change the power state?
            console.log(Console::INFO, F("Shutting down PC..."));
            shutDownPC();
            changedPowerState = false; // reset
          }
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