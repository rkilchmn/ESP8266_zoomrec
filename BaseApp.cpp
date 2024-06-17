
#include "BaseApp.h"

#include "features.h"
#include <Arduino.h>
#include <stdarg.h>

#ifdef GDB_DEBUG
#include <GDBStub.h>
#endif

BaseApp::BaseApp()
{
}

BaseApp::~BaseApp()
{
#ifdef CONSOLE_TELNET
  if (pBufferedTelnetStream != nullptr)
  {
    delete pBufferedTelnetStream;
  }
#endif

#ifdef CONSOLE_HTTP
  if (pBufferedHTTPRestStream != nullptr)
  {
    delete pBufferedHTTPRestStream;
  }
#endif
}

const char* BaseApp::getMDNSHostname() {
    return "ESP8266-BaseApp";
  }

void BaseApp::AppFirmwareVersion()
{
  FIRMWARE_VERSION = String(__FILE__) + "-" + String(__DATE__) + "-" + String(__TIME__);
}

/* Watchdog to guard against the ESP8266 wasting battery power looking for
 *  non-responsive wifi networks and servers. Expiry of the watchdog will trigger
 *  either a deep sleep cycle or a delayed reboot. The ESP8266 OS has another built-in
 *  watchdog to protect against infinite loops and hangups in user code.
 */

void BaseApp::timeoutCallback()
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
void BaseApp::flash()
{
  digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
}
#endif

#ifdef WIFI_PORTAL
// Callback for entering config mode
void BaseApp::configModeCallback(WiFiManager *myWiFiManager)
{
  // Config mode has its own timeout
  watchdog.detach();

#ifdef LED_STATUS_FLASH
  flasher.attach(0.2, [this]()
                 { flash(); });
#endif
}
#endif

#ifdef USE_NTP
void BaseApp::setupNtp()
{
  settimeofday_cb([this](boolean from_sntp)
                  { time_is_set(from_sntp); }); // optional: callback if time was sent
  configTime(config.get("timezone_ntp", TZ_Europe_London), NTP_SERVER);
}

void BaseApp::time_is_set(boolean from_sntp /* <= this optional parameter can be used with ESP8266 Core 3.0.0*/)
{
  ntp_set = true;

  time_t now = time(nullptr);
  // causes dumps with HTTP CONSOLE???
  // console.log(Console::INFO, F("NTP time received from_sntp=%s"), from_sntp ? "true" : "false");
  // console.log(Console::INFO, F("Current local time: %s"), ctime(&now));
}

uint32_t BaseApp::sntp_startup_delay_MS_rfc_not_less_than_60000()
{
  randomSeed(A0);
  return random(5000);
}

uint32_t BaseApp::sntp_update_delay_MS_rfc_not_less_than_15000()
{
  return 12 * 60 * 60 * 1000UL; // 12 hours
}
#endif

#ifdef USE_MDNS
void BaseApp::setupMDNS() {
  // Start the mDNS responder
  const char* mDNSHostname = getMDNSHostname();
  mDNSHostname = config.get("mDNSHostname", mDNSHostname);
  if (MDNS.begin(mDNSHostname)) {  // Initialize mDNS with the hostname
    console.log(Console::INFO, F("Advertising mDNS with hostname='%s.local'"), mDNSHostname);
  } else {
    console.log(Console::ERROR, F("Advertising mDNS with hostname='%s.local' failed"), mDNSHostname);
  }
}
#endif

#ifdef ARDUINO_OTA
void BaseApp::setupArduinoOta()
{
  // Arduino OTA Initalisation
  int port = config.get("arduino_ota_port", ARDUINO_OTA_PORT);
  ArduinoOTA.setPort(port);
#ifdef USE_MDNS // needs to done again even if MSDN.begin ws already calleed
  const char* mDNSHostname = getMDNSHostname();
  mDNSHostname = config.get("mDNSHostname", mDNSHostname);
  ArduinoOTA.setHostname( mDNSHostname);
#endif

  ArduinoOTA.setPassword(config.get("arduino_ota_password", ARDUINO_OTA_PASSWD));

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
  console.log(Console::INFO, F("Started Arduino OTA Server on port: %d"), port);
}
#endif

#ifdef HTTP_OTA
boolean BaseApp::performHttpOtaUpdate()
{
  String http_ota_url = config.get("http_ota_url", HTTP_OTA_URL);
  String http_ota_username = config.get("http_ota_username", HTTP_OTA_USERNAME);
  String http_ota_password = config.get("http_ota_password", HTTP_OTA_PASSWORD);

  // Check server for firmware updates
  console.log(Console::INFO, F("Checking for firmware update via HTTP OTA from %s"), http_ota_url.c_str());

  WiFiClient client;
  client.setTimeout(30000); // 

#ifdef LED_STATUS_FLASH
  ESPhttpUpdate.setLedPin(STATUS_LED, LED_ON); // define level for LED on
#endif

  // use authorization?
  if (!http_ota_username.isEmpty() && !http_ota_password.isEmpty())
    ESPhttpUpdate.setAuthorization(http_ota_username, http_ota_password);

  watchdog.detach();
  switch (ESPhttpUpdate.update(client, http_ota_url, FIRMWARE_VERSION))
  {
  case HTTP_UPDATE_FAILED:
    console.log(Console::WARNING, F("Firmware update via HTTP OTA failed: Code (%d) %s"), ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
    return false;

  case HTTP_UPDATE_NO_UPDATES:
    console.log(Console::INFO, F("No firmware update available via HTTP OTA "));
    return false;

  case HTTP_UPDATE_OK:
    console.log(Console::INFO, F("Firmware update via HTTP OTA successfull"));
    return true;
  default:
    return false;
  }
  return false;
}
#endif

boolean BaseApp::connectWiFi()
{
#ifdef LED_STATUS_FLASH
  pinMode(STATUS_LED, OUTPUT);
  flasher.attach(0.6, [this]()
                 { flash(); });
#endif

  // Watchdog timer - resets if setup takes longer than allocated time
  watchdog.once(WATCHDOG_SETUP_SECONDS, [this]()
                { timeoutCallback(); });

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
      wifiManager.setAPCallback([this](WiFiManager *myWiFiManager)
                                { configModeCallback(myWiFiManager); });
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

String BaseApp::getResetReasonString(uint8_t reason)
{
  switch (reason)
  {
  case REASON_DEFAULT_RST:
    return "Power-on reset";
  case REASON_WDT_RST:
    return "Watchdog reset";
  case REASON_EXCEPTION_RST:
    return "Exception reset";
  case REASON_SOFT_WDT_RST:
    return "Software Watchdog reset";
  case REASON_SOFT_RESTART:
    return "Software restart";
  case REASON_DEEP_SLEEP_AWAKE:
    return "Deep sleep wake-up";
  case REASON_EXT_SYS_RST:
    return "External system reset";
  default:
    return "Unknown reset reason";
  }
}

// Put any project specific code here
const int INPUTPINRESETSWITCH = D1;
const int outputPinPowerButton = D2;
int lastStatus = LOW;

void BaseApp::setup()
{
#ifdef GDB_DEBUG
  gdbstub_init();
#endif

  // try to initialize with baud rate from config
  int serial_baud = config.get("serial_baud", SERIAL_DEFAULT_BAUD);
  console.begin(serial_baud);

  // determine logLevel
  int logLevel = config.get("log_level", Console::DEBUG);
  console.setLogLevel(console.intToLogLevel(logLevel));

  console.println(); // newline after garbage from startup
  AppFirmwareVersion();
  console.log(Console::INFO, F("Current firmware version: '%s'"), (FIRMWARE_VERSION).c_str());
  logEnabledFeatures();

  console.log(Console::DEBUG, F("Start of initialization: Reset Reason='%s'"), getResetReasonString(ESP.getResetInfoPtr()->reason).c_str());

  // setup for deep sleep
  pinMode(D0, WAKEUP_PULLUP); // be carefull when using D0 and using deep sleep
  int deep_sleep_option = config.get("deep_sleep_option", WAKE_RFCAL);
  system_deep_sleep_set_option(deep_sleep_option);

  // connect to WIFI depending on what connection features are enabled
  connectWiFi();

#ifdef CONSOLE_TELNET
  int port = config.get("telnet_port", TELNET_DEFAULT_PORT);
  console.log(Console::INFO, F("Telnet service started on port: %d"), port);
  pBufferedTelnetStream = new TelnetStreamBuffered(port);
  pBufferedTelnetStream->begin(port);
  console.begin(*pBufferedTelnetStream, Serial); // continue output to Serial
#else
#ifdef CONSOLE_HTTP
  // int port = config.get("telnet_port", TELNET_DEFAULT_PORT);
  // console.log(Console::INFO, F("Telnet service started on port: %d"), port);
  const char *http_log_url = config.get("http_log_url");
  if (!strlen(http_log_url))
  { // empty
    console.log(Console::ERROR, F("Missing configuration for 'http_log_url'. HTTP logging not started."));
  }
  else
  {
    pBufferedHTTPRestStream = new HttpStreamBuffered(
        config.get("http_log_id", FIRMWARE_VERSION.c_str()), http_log_url, "/log",
        config.get("http_log_username"), config.get("http_log_password"));
    console.log(Console::INFO, F("Starting HTTP logging to %s."), http_log_url);
    console.begin(*pBufferedHTTPRestStream, Serial);
  }
#endif
#endif

  // Print config
  config.print(&console);

#ifdef USE_MDNS
  setupMDNS();
#endif

#ifdef ARDUINO_OTA
  setupArduinoOta();
#endif

#ifdef JSON_CONFIG_OTA
  config.setupOtaServer(&console);
#endif

#ifdef HTTP_OTA
  performHttpOtaUpdate();
#endif

#ifdef USE_NTP
  setupNtp();
#endif

#ifdef DEEP_SLEEP_SECONDS
  if (!deepSleepState.loadFromRTC())
  {
    console.log(Console::DEBUG, F("DeepSleepState cold boot - calling appDeepSleepStateInit for state initialization."));
    AppDeepSleepStateInit();
  }
#endif

  // individual setup for apps
  AppSetup();

  console.log(Console::DEBUG, F("End of initialization"));
  watchdog.detach();
}

void BaseApp::AppSetup()
{
  // override this method if required
}

void BaseApp::AppLoop()
{
  // override this method if required
}

void BaseApp::AppIntervall()
{
  // override this method if required
}

void BaseApp::AppDeepSleepStateInit()
{
  // override this method if required
}

void BaseApp::loop()
{

#ifdef USE_MDNS
  // Handle mDNS requests
  MDNS.update();  // Keep the mDNS responder active
#endif

#ifdef JSON_CONFIG_OTA
  // Handle HTTP requests
  config.handleOTAServerClient();
#endif

#ifdef ARDUINO_OTA
  // Handle any OTA upgrade
  ArduinoOTA.handle();
#endif

  // Watchdog timer - resets if setup takes longer than allocated time
  watchdog.once(WATCHDOG_LOOP_SECONDS, [this]()
                { timeoutCallback(); });

#ifdef TIMER_INTERVAL_MILLIS
  // https://www.norwegiancreations.com/2018/10/arduino-tutorial-avoiding-the-overflow-issue-when-using-millis-and-micros/
  if ((unsigned long)(millis() - timer_interval) > TIMER_INTERVAL_MILLIS)
  {
    timer_interval = millis();
    AppIntervall();
  }
#endif

  AppLoop();

  watchdog.detach();

#ifdef DEEP_SLEEP_SECONDS

  // no deep sleep after normal power up to allow for OTA updates
  bool expired = false;
  if ((unsigned long)(millis() - timer_coldboot) > DEEP_SLEEP_STARTUP_SECONDS * 1000)
  {
    timer_coldboot = millis();
    expired = true;
  }

  if ((ESP.getResetInfoPtr()->reason == REASON_DEEP_SLEEP_AWAKE) || expired)
  {
    if (preventDeepSleep)
      console.log(Console::DEBUG, F("Prevented from deep sleep: preventDeepSleep=%d"), preventDeepSleep);
    else
    {
      // Enter DeepSleep
      deepSleepState.saveToRTC();
      console.log(Console::DEBUG, F("Entering deep sleep for %d seconds..."), DEEP_SLEEP_SECONDS);
      console.flush();
      ESP.deepSleep(DEEP_SLEEP_SECONDS * 1000000, WAKE_RF_DEFAULT);
      // Do nothing while we wait for sleep to overcome us
      while (true)
      {
      }
    }
  }
#endif
}

// Function to log enabled features
void BaseApp::logEnabledFeatures() {
    // Check and log each 
    #ifdef WIFI_PORTAL
    console.log(Console::INFO, F("Feature Enabled: WiFi Configuration Portal"));
    #endif

    #ifdef WPS_CONFIG
    console.log(Console::INFO, F("Feature Enabled: WPS Configuration"));
    #endif

    #ifdef ARDUINO_OTA
    console.log(Console::INFO, F("Feature Enabled: Arduino OTA Updates"));
    #endif

    #ifdef HTTP_OTA
    console.log(Console::INFO, F("Feature Enabled: HTTP OTA Updates"));
    #endif

    #ifdef LED_STATUS_FLASH
    console.log(Console::INFO, F("Feature Enabled: LED Status Flash"));
    #endif

    #ifdef DEEP_SLEEP_SECONDS
    console.log(Console::INFO, F("Feature Enabled: Deep Sleep Mode (Seconds: %d, Startup Seconds: %d)"), DEEP_SLEEP_SECONDS, DEEP_SLEEP_STARTUP_SECONDS);
    #endif

    #ifdef JSON_CONFIG_OTA
    console.log(Console::INFO, F("Feature Enabled: JSON Configuration OTA"));
    #endif

    #ifdef GDB_DEBUG
    console.log(Console::INFO, F("Feature Enabled: GDB Debugging"));
    #endif

    #ifdef USE_NTP
    console.log(Console::INFO, F("Feature Enabled: NTP (Network Time Protocol)"));
    #endif

    #ifdef USE_MDNS
    console.log(Console::INFO, F("Feature Enabled: mDNS (Local Network Hostname Resolution)"));
    #endif

    #ifdef TIMER_INTERVAL_MILLIS
    console.log(Console::INFO, F("Feature Enabled: Timer Interval Execution (ms: %d)"), TIMER_INTERVAL_MILLIS);
    #endif

    #ifdef CONSOLE_TELNET
    console.log(Console::INFO, F("Feature Enabled: Telnet Console Output"));
    #endif

    #ifdef CONSOLE_HTTP
    console.log(Console::INFO, F("Feature Enabled: HTTP Console Output"));
    #endif
}