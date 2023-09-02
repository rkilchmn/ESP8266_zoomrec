
#include "AppBase.h"

#include "features.h"
#include <Arduino.h>
#include <stdarg.h>

#ifdef GDB_DEBUG
    #include <GDBStub.h>
#endif

AppBase::AppBase() {
    // constructor implementation
}

AppBase::~AppBase() {
    // destructor implementation
}

/* Watchdog to guard against the ESP8266 wasting battery power looking for
 *  non-responsive wifi networks and servers. Expiry of the watchdog will trigger
 *  either a deep sleep cycle or a delayed reboot. The ESP8266 OS has another built-in
 *  watchdog to protect against infinite loops and hangups in user code.
 */

void AppBase::timeoutCallback()
{
  // This sleep happened because of timeout. Do a restart after a sleep
  console.log(Console::CRITICAL, F("Watchdog timeout...restarting"));
  console.flush();

#ifdef DEEP_SLEEP_SECONDS
  // Enter DeepSleep so that we don't exhaust our batteries by countinuously trying to
  // connect to a network that isn't there.
  ESP.deepSleep(DEEP_SLEEP_SECONDS * 1000, WAKE_RF_DEFAULT);
  // Do nothing while we wait for sleep to overcome us
  while (true)
  {
  };

#else
  delay(1000);
  ESP.restart();
#endif
}

#ifdef LED_STATUS_FLASH
    void AppBase::flash()
    {
    digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
    }
#endif

#ifdef WIFI_PORTAL
// Callback for entering config mode
void AppBase::configModeCallback(WiFiManager *myWiFiManager)
    {
    // Config mode has its own timeout
    watchdog.detach();

    #ifdef LED_STATUS_FLASH
        flasher.attach(0.2, [this]() {flash();});
    #endif
    }
#endif

#ifdef USE_NTP
    void AppBase::setup_NTP()
    {
    settimeofday_cb([this](boolean from_sntp) {time_is_set(from_sntp);}); // optional: callback if time was sent
    configTime( config.useConfig("timezone_ntp", TZ_Europe_London), NTP_SERVER);
    }

    void AppBase::time_is_set(boolean from_sntp /* <= this optional parameter can be used with ESP8266 Core 3.0.0*/)
    {
    ntp_set = true;

    time_t now = time(nullptr);
    console.log(Console::INFO, F("NTP time received from_sntp=%s"), from_sntp ? "true" : "false");
    console.log(Console::INFO, F("Current local time: %s"), ctime(&now));
    }

    uint32_t AppBase::sntp_startup_delay_MS_rfc_not_less_than_60000()
    {
    randomSeed(A0);
    return random(5000);
    }

    uint32_t AppBase::sntp_update_delay_MS_rfc_not_less_than_15000()
    {
    return 12 * 60 * 60 * 1000UL; // 12 hours
    }
#endif


#ifdef ARDUINO_OTA
void AppBase::setupArduinoOTA()
{
  // Arduino OTA Initalisation
  ArduinoOTA.setPort(ARDUINO_OTA_PORT);
  ArduinoOTA.setHostname(SSID);
  ArduinoOTA.setPassword(ARDUINO_OTA_PASSWD);
  ArduinoOTA.onStart([this]()
                     {
      watchdog.detach();
      console.log(Console::INFO, F("OTA upload starting...")); });
  ArduinoOTA.onEnd([this]()
                   { console.log(Console::INFO, F("OTA upload finished")); });
  ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total)
                        { console.log(Console::INFO, F("Progress: %u%%"), (progress / (total / 100))); });
  ArduinoOTA.onError([this](ota_error_t error)
                     {
      console.log(Console::ERROR, F("Error[%u]: "), error);
      if (error == OTA_AUTH_ERROR) console.log(Console::ERROR, F("Auth Failed"));
      else if (error == OTA_BEGIN_ERROR) console.log(Console::ERROR, F("Begin Failed"));
      else if (error == OTA_CONNECT_ERROR) console.log(Console::ERROR, F("Connect Failed"));
      else if (error == OTA_RECEIVE_ERROR) console.log(Console::ERROR, F("Receive Failed"));
      else if (error == OTA_END_ERROR) console.log(Console::ERROR, F("End Failed")); });
  ArduinoOTA.begin();
}
#endif

#ifdef HTTP_OTA
    boolean AppBase::perform_HTTP_OTA_Update()
    {
    String http_ota_url = config.useConfig("http_ota_url", HTTP_OTA_URL);
    String http_ota_username = config.useConfig("http_ota_username", HTTP_OTA_USERNAME);
    String http_ota_password = config.useConfig("http_ota_password", HTTP_OTA_PASSWORD);

    // Check server for firmware updates
    console.log(Console::INFO, F("Checking for firmware updates from %s"), http_ota_url.c_str());

    WiFiClient client;
    #ifdef LED_STATUS_FLASH
    ESPhttpUpdate.setLedPin(STATUS_LED, LED_ON); // define level for LED on
    #endif

    // use authorization?
    if (!http_ota_username.isEmpty() && !http_ota_password.isEmpty())
        ESPhttpUpdate.setAuthorization(http_ota_username, http_ota_password);

    switch (ESPhttpUpdate.update(client, http_ota_url, HTTP_OTA_VERSION))
    {
    case HTTP_UPDATE_FAILED:
        console.log(Console::WARNING, F("HTTP update failed: Code (%d) %s"), ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        return false;

    case HTTP_UPDATE_NO_UPDATES:
        console.log(Console::INFO, F("No updates"));
        return false;

    case HTTP_UPDATE_OK:
        console.log(Console::INFO, F("Update OK"));
        return true;
    default:
        return false;
    }
    return false;
    }
#endif

boolean AppBase::connectWiFi()
{
#ifdef LED_STATUS_FLASH
  pinMode(STATUS_LED, OUTPUT);
  flasher.attach(0.6, [this]() {flash();});
#endif

  // Watchdog timer - resets if setup takes longer than allocated time
  watchdog.once(WATCHDOG_SETUP_SECONDS, [this]() {timeoutCallback();});

  // try connect using previous connection details stored in eeprom
  if (WiFi.SSID().length() > 0)
  {
    // Print the SSID and password
    console.log(Console::INFO, F("WiFi credentials stored: %s"), WiFi.SSID().c_str());

    console.log(Console::INFO, F("Connecting..."));
    
    WiFi.begin(WiFi.SSID(), WiFi.psk());
    WiFi.waitForConnectResult(FAST_CONNECTION_TIMEOUT);
  }

#ifdef WPS_CONFIG
  if (WiFi.status() != WL_CONNECTED)
  {
    console.log(Console::INFO, F("Starting WPS configuration..."));
    WiFi.beginWPSConfig();

    if (WiFi.SSID().length() > 0)
    {
      // Print the SSID and password
      console.printf("WPS credentials received: %s", WiFi.SSID().c_str());

      WiFi.begin(WiFi.SSID(), WiFi.psk());
      WiFi.persistent(true);
      WiFi.setAutoConnect(true);
      WiFi.setAutoReconnect(true);
      if (WiFi.waitForConnectResult() != WL_CONNECTED)
      {
        console.log(Console::WARNING, F("Connecting using WPS timed out!"));
        timeoutCallback();
      }
    }
  }
#endif

  // Set up WiFi connection using portal
  if (WiFi.status() != WL_CONNECTED)
  {
#ifdef WIFI_PORTAL
#ifdef WIFI_PORTAL_TRIGGER_PIN
    pinMode(WIFI_PORTAL_TRIGGER_PIN, INPUT_PULLUP);
    delay(100);
    if (digitalRead(WIFI_PORTAL_TRIGGER_PIN) == LOW)
    {
      watchdog.detach();
      if (!wifiManager.startConfigPortal(SSID, NULL))
      {
        console.log(Console::ERROR, F("Starting WiFi Config Portal Failed!"));
        timeout_cb();
      }
    }
    else
    {
#endif

      // for debugging
      wifiManager.setDebugOutput(true);

      wifiManager.setConfigPortalTimeout(180);
      wifiManager.setAPCallback([this](WiFiManager *myWiFiManager) {configModeCallback(myWiFiManager);});
      if (!wifiManager.autoConnect())
      {
        console.log(Console::WARNING, F("Connection Failed!"));
        timeoutCallback();
      }

#ifdef WIFI_PORTAL_TRIGGER_PIN
    }
#endif

#else
    // Save boot up time by not configuring them if they haven't changed
    if (WiFi.SSID() != WIFI_SSID)
    {
      console.log(Console::INFO, F("Initialising Wifi..."));
      WiFi.mode(WIFI_STA);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      WiFi.persistent(true);
      WiFi.setAutoConnect(true);
      WiFi.setAutoReconnect(true);
    }
#endif
  }

  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    console.log(Console::WARNING, F("Connection Finally Failed!"));
    timeoutCallback();
  }

#ifdef LED_STATUS_FLASH
  flasher.detach();
  digitalWrite(STATUS_LED, LED_OFF);
#endif

  if (WiFi.status() == WL_CONNECTED)
  {
    // we have internet connection
    console.log(Console::INFO, F("IP address: %s"), WiFi.localIP().toString().c_str());
    return true;
  }
  else
    return false;
}

String AppBase::getResetReasonString(uint8_t reason) {
  switch (reason) {
    case REASON_DEFAULT_RST:    return "Power-on reset";
    case REASON_WDT_RST:        return "Watchdog reset";
    case REASON_EXCEPTION_RST:  return "Exception reset";
    case REASON_SOFT_WDT_RST:   return "Software Watchdog reset";
    case REASON_SOFT_RESTART:   return "Software restart";
    case REASON_DEEP_SLEEP_AWAKE: return "Deep sleep wake-up";
    case REASON_EXT_SYS_RST:    return "External system reset";
    default:                    return "Unknown reset reason";
  }
}


// Put any project specific code here
const int inputPinResetSwitch = D1;
const int outputPinPowerButton = D2;
int lastStatus = LOW;

void AppBase::setup()
{
    #ifdef GDB_DEBUG
    gdbstub_init();
    #endif

    // try to initialize with baud rate from config
    int serial_baud = config.useConfig("serial_baud", SERIAL_DEFAULT_BAUD);
    console.begin( serial_baud); 

    // determine logLevel
    int logLevel = config.useConfig("log_level", Console::DEBUG);
    console.setLogLevel( console.intToLogLevel( logLevel));
  
  console.println(); // newline after garbage from startup
  console.log(Console::DEBUG, F("Start of initialization: Reset Reason='%s'"), getResetReasonString(ESP.getResetInfoPtr()->reason).c_str()); 

  // setup for deep sleep
  pinMode(D0, WAKEUP_PULLUP); // be carefull when using D0 and using deep sleep
  int deep_sleep_option = config.useConfig("deep_sleep_option", WAKE_RFCAL);
  system_deep_sleep_set_option( deep_sleep_option);

  // connect to WIFI depending on what connection features are enabled
  connectWiFi();

#ifdef TELNET
  int port = config.useConfig("telnet_port", TELNET_DEFAULT_PORT);
  console.log(Console::INFO, F("Telnet service started on port: %d"), port);
  BufferedTelnetStream.begin(port);
  console.begin(BufferedTelnetStream, Serial); // continue output to Serial
#endif

  // Print config
  config.print();

#ifdef HTTP_OTA
  // remove watchdog while updating
  watchdog.detach();
  perform_HTTP_OTA_Update();
  // Watchdog timer - resets if setup takes longer than allocated time
  watchdog.once(WATCHDOG_SETUP_SECONDS, [this]() {timeoutCallback();});
#endif

#ifdef ARDUINO_OTA
  setupArduinoOTA();
#endif

#ifdef JSON_CONFIG_OTA
  config.setup_JSON_CONFIG_OTA();
#endif

#ifdef USE_NTP
  setup_NTP();
#endif

#ifdef HTTP_OTA
  perform_HTTP_OTA_Update();
#endif

  console.log(Console::INFO, F("Current firmware version: '%s'"), (HTTP_OTA_VERSION).c_str());

  // individual setup for apps
  SetupApp();

  console.log(Console::DEBUG, F("End of initialization"));
  watchdog.detach();
}

 void AppBase::SetupApp() {
    // override this method if required
 }

  void AppBase::LoopApp(){
    // override this method if required
  }

  void AppBase::IntervalApp(){
    // override this method if required
  }

void AppBase::loop()
{

#ifdef ARDUINO_OTA
  // Handle any OTA upgrade
  ArduinoOTA.handle();
#endif

#ifdef JSON_CONFIG_OTA
  // Handle HTTP requests
  config.handleRequest();
#endif

  // Watchdog timer - resets if setup takes longer than allocated time
  watchdog.once(WATCHDOG_LOOP_SECONDS, [this]() {timeoutCallback();});

#ifdef USE_NTP
  // do not start processing main loop until NTP time is set
  if (ntp_set) {

#ifdef TIMER_INTERVAL_MILLIS
  // https://www.norwegiancreations.com/2018/10/arduino-tutorial-avoiding-the-overflow-issue-when-using-millis-and-micros/
  if ((unsigned long)(millis() - timer_interval) > TIMER_INTERVAL_MILLIS)
  {
    timer_interval = millis();
    IntervalApp();
  }
#endif

  // apps loop
  LoopApp();

  watchdog.detach();

#ifdef DEEP_SLEEP_SECONDS

  // no deep sleep after normal power up to allow for OTA updates
  bool expired = false;
  if ((unsigned long)(millis() - timer_startup) > DEEP_SLEEP_STARTUP_SECONDS * 1000) {
    timer_startup = millis();
    expired = true;
  }

  if ((ESP.getResetInfoPtr()->reason == REASON_DEEP_SLEEP_AWAKE) || expired) {
    // Enter DeepSleep
    console.log(Console::DEBUG, F("Entering deep sleep for %d seconds..."), DEEP_SLEEP_SECONDS);
    ESP.deepSleep(DEEP_SLEEP_SECONDS * 1000000, WAKE_RF_DEFAULT);
    // Do nothing while we wait for sleep to overcome us
    while (true)
    {
    };
  }
#endif
  }
#endif
}