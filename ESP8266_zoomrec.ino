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
  
  void AppNTPSet()
  {
    time_t now = time(nullptr);
    console.log(Console::INFO, F("NTP time received: %s"), ctime(&now));
  }

  void AppDeepSleepStateInit()
  {
    changedPowerState = false;
    console.log(Console::DEBUG, F("DeepSleepState cold boot - initializing: changedPowerState=%d"), changedPowerState);
  }

  void AppSetup()
  {
    // Put your initialisation code here
    // pinMode(INPUTPINRESETSWITCH, INPUT_PULLUP);
    pinMode(INPUTPINRESETSWITCH, INPUT);
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
      // call get_next_event api
      char path[200];
      snprintf(path, sizeof(path), "/event/next?client_id=%s&lead_time_sec=%d&trail_time_sec=%d",
        urlEncode(config.get("client_id", "")).c_str(), config.get("leadin_secs", 60), config.get("leadout_secs", 60));

      DynamicJsonDocument responseBody(2048); 
      StaticJsonDocument<0> emptyRequestHeader;
      StaticJsonDocument<0> emptyRequestBody;

      int httpCode = JSONAPIClient::performRequest(
        *client,
        JSONAPIClient::HTTP_METHOD_GET, 
        config.get("http_api_base_url", ""), 
        path,
        emptyRequestHeader,
        emptyRequestBody,
        responseBody,
        config.get("http_api_username", ""), 
        config.get("http_api_password", "")
      );

      const int EVENT_ONGOING_UNKNOWN = -1;
      const int EVENT_NOT_ONGOING = 0;
      const int EVENT_ONGOING = 1;
      
      int eventOngoing = EVENT_ONGOING_UNKNOWN;

      switch (httpCode)
      {
        case HTTP_CODE_OK:
          if (responseBody.size() > 0) {
            console.log(Console::DEBUG, F("response for /event/next: "));
            serializeJsonPretty(responseBody, console);
            console.println();
            // extract
            char startStr[50];
            char endStr[50];
            char nowStr[50];

            // Check if required fields exist in the response
            if (!responseBody.containsKey("dtstart_instance_lead") || 
                !responseBody.containsKey("dtend_instance_trail") || 
                !responseBody.containsKey("dtnow")) {
              console.log(Console::ERROR, F("Missing required fields in response"));
              eventOngoing = EVENT_ONGOING_UNKNOWN;
            }
            else
            {
              strcpy(startStr, responseBody["dtstart_instance_lead"]);
              strcpy(endStr, responseBody["dtend_instance_trail"]);
              strcpy(nowStr, responseBody["dtnow"]);

              // Additional check for empty strings (though the above check should handle this)
              if ((strlen(startStr) == 0) || (strlen(endStr) == 0) || (strlen(nowStr) == 0))
              {
                console.log(Console::ERROR, F("response for '/event/next' does not contain dtstart_instance_lead, dtend_instance_trail or dtnow"));
                eventOngoing = EVENT_ONGOING_UNKNOWN;
              }
              else
              {
                // Convert ISO 8601 timestamps to time_t (Unix timestamp)
                time_t startTime = convertISO8601ToUnixTime(startStr);
                time_t endTime = convertISO8601ToUnixTime(endStr);
                time_t currentTime = convertISO8601ToUnixTime(nowStr);

                // Check if the event has started and has not yet ended
                if ((currentTime >= startTime) && (currentTime <= endTime))
                  eventOngoing = EVENT_ONGOING;
                else
                  eventOngoing = EVENT_NOT_ONGOING;
              }
            }
          }
          else  
          {
            console.log(Console::WARNING, F("response with status code %d for '/event/next' is empty"), httpCode);
            eventOngoing = EVENT_ONGOING_UNKNOWN;
          }
          break;
        case HTTP_CODE_NO_CONTENT:
          console.log(Console::DEBUG, F("response with status code %d for '/event/next' is empty"), httpCode);
          eventOngoing = EVENT_NOT_ONGOING; // no next event but need to check postprocessing
          break;
        default:
          console.log(Console::WARNING, F("Call to '/event/next' failed with httpCode=%d"), httpCode);
          eventOngoing = EVENT_ONGOING_UNKNOWN;
          break;
      }

      if (eventOngoing == EVENT_NOT_ONGOING or eventOngoing == EVENT_ONGOING_UNKNOWN) {
        const int EVENT_STATUS_POSTPROCESSING = 3;
        // call get api with status postprocessing and this client_id
        snprintf(path, sizeof(path), "/event?Filter.1.Name=status&Filter.1.Operator=%s&Filter.1.Value=%d&Filter.2.Name=assigned&Filter.2.Operator=%s&Filter.2.Value=%s&fields=status",
          urlEncode("="), EVENT_STATUS_POSTPROCESSING, urlEncode("="), urlEncode(config.get("client_id", "")).c_str());

        // Reuse the same TLS settings as the first request
        httpCode = JSONAPIClient::performRequest(
          *client,
          JSONAPIClient::HTTP_METHOD_GET, 
          config.get("http_api_base_url", ""), 
          path,
          emptyRequestHeader, 
          emptyRequestBody, 
          responseBody,
          config.get("http_api_username", ""), 
          config.get("http_api_password", "")
        );

        switch (httpCode) {
          case HTTP_CODE_OK:
            if (responseBody.size() > 0) {
              console.log(Console::DEBUG, F("response for /event/get: "));
              serializeJsonPretty(responseBody, console);
              console.println();
  
              // still in postprocessing
              eventOngoing = EVENT_ONGOING;
            }
            else {
              console.log(Console::WARNING, F("response with status code %d for '/event/get' is empty"), httpCode);
              eventOngoing = EVENT_ONGOING_UNKNOWN;
            }
            break;
          case HTTP_CODE_NO_CONTENT:
            console.log(Console::DEBUG, F("response with status code %d for '/event/get' is empty"), httpCode);
            switch (eventOngoing) {
              case EVENT_NOT_ONGOING:
                eventOngoing = EVENT_NOT_ONGOING; // there was no next event ongoing and also no postprocessing
                break;
              case EVENT_ONGOING_UNKNOWN:
                eventOngoing = EVENT_ONGOING_UNKNOWN; // unsure if there was next event ongoing and there is no postprocessing
                break;
              default:
                eventOngoing = EVENT_ONGOING_UNKNOWN;
                break;
            }
            break;
          default:
            console.log(Console::WARNING, F("Call to '/event/get' failed with httpCode=%d"), httpCode);
            eventOngoing = EVENT_ONGOING_UNKNOWN;
            break;
        }
      }

      // update PC power status
      switch (eventOngoing)
      { 
      case EVENT_ONGOING:
        console.log(Console::INFO, F("Event ongoing..."));
        if (getPowerState() == PC_OFF)
        {
          console.log(Console::INFO, F("Starting PC..."));
          startPC();
          changedPowerState = true; // we changed power state
        }
        break;
      case EVENT_NOT_ONGOING:
        console.log(Console::INFO, F("No event ongoing..."));
        if (getPowerState() == PC_ON)
        {
          if (changedPowerState) { // did we change the power state?
            console.log(Console::INFO, F("Shutting down PC..."));
            shutDownPC();
            changedPowerState = false; // reset
          }
        }
        break;
      case EVENT_ONGOING_UNKNOWN:
        console.log(Console::WARNING, F("Status of event ongoing unknown. No action taken."));
        break;
      default:
        console.log(Console::ERROR, F("Invalid eventOngoing value: %d"), eventOngoing);
        break;
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