#include <Arduino.h>
#include <UrlEncode.h>
#include <ArduinoJson.h>

#include "BaseApp.h"
#include "JSONAPIClient.h"  
#include "iso_date.h"

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

  const char* getMDNSHostname() {
    return "ESP8266-Zoomrec";
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
      bool eventOngoing = false;  

      // call get_next_event api
      char path[200];
      snprintf(path, sizeof(path), "/event/next?client_id=%s&lead_time_sec=%d&trail_time_sec=%d",
        urlEncode(config.get("client_id", "")).c_str(), config.get("leadin_secs", 60), config.get("leadout_secs", 60));

      DynamicJsonDocument responseBody(2048);  // Use a larger value
      StaticJsonDocument<0> emptyRequestHeader;
      StaticJsonDocument<0> emptyRequestBody;

      int httpCode = JSONAPIClient::performRequest(
        JSONAPIClient::HTTP_METHOD_GET, config.get("http_api_base_url", ""), path,
        emptyRequestHeader, emptyRequestBody, responseBody,
        config.get("http_api_username", ""), config.get("http_api_password", ""), ""
      );

      if (httpCode == HTTP_CODE_OK && responseBody.size() > 0) {
        console.log(Console::DEBUG, F("response for /event/next: "));
        serializeJsonPretty(responseBody, console);
        console.println();
        // extract
        char startStr[50];
        char endStr[50];
        char nowStr[50];

        strcpy(startStr, responseBody["dtstart_instance_lead"]);
        strcpy(endStr, responseBody["dtend_instance_trail"]);
        strcpy(nowStr, responseBody["dtnow"]);

        if ((startStr == nullptr || *startStr == '\0') ||
            (endStr == nullptr || *endStr == '\0') ||
            (nowStr == nullptr || *nowStr == '\0')
          ) 
        {
          console.log(Console::DEBUG, F("response does not contain dtstart_instance_lead, dtend_instance_trail or dtnow"));
        }
        else
        {

          // Convert ISO 8601 timestamps to time_t (Unix timestamp)
          time_t startTime = convertISO8601ToUnixTime(startStr);
          time_t endTime = convertISO8601ToUnixTime(endStr);
          time_t currentTime = convertISO8601ToUnixTime(nowStr);

          console.log(Console::DEBUG, F("startStr: %s startTime: %ld"), startStr, startTime);
          console.log(Console::DEBUG, F("endStr: %s endTime: %ld"), endStr, endTime);
          console.log(Console::DEBUG, F("nowStr: %s currentTime: %ld"), nowStr, currentTime);

          // Check if the event has started and has not yet ended
          if ((currentTime >= startTime) && (currentTime <= endTime))
            eventOngoing = true;
        }
      }
      else {
        // something failed, already logged in getJsonApiResponse
      }

      if (!eventOngoing) {
        // call get api with status postprocessing and this client_id
        snprintf(path, sizeof(path), "/event?Filter.1.Name=status&Filter.1.Operator=%s&Filter.1.Value=%d&?Filter.2.Name=assigned&Filter.2.Operator=%s&Filter.2.Value=%s",
          urlEncode("="), 3, urlEncode("="), urlEncode(config.get("client_id", "")).c_str());

        httpCode = JSONAPIClient::performRequest(
          JSONAPIClient::HTTP_METHOD_GET, config.get("http_api_base_url", ""), path,
          emptyRequestHeader, emptyRequestBody, responseBody,
          config.get("http_api_username", ""), config.get("http_api_password", ""), ""
        );

        if (httpCode == HTTP_CODE_OK && responseBody.size() > 0) {
          console.log(Console::DEBUG, F("response for /event/get: "));
          serializeJsonPretty(responseBody, console);
          console.println();

          // still in postprocessing
          eventOngoing = true;
        }
        else {
          // something failed, already logged in getJsonApiResponse
        }
      }

      // update PC power status
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