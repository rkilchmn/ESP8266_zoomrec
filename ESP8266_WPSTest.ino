#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

#include <GDBStub.h>

#define LED_ON LOW               // Define the value to turn the LED on
#define LED_OFF HIGH             // Define the value to turn the LED off
#define CONNECTION_TIMEOUT 10000 // connection timeout in milli seconds

void flashLED()
{
  digitalWrite(LED_BUILTIN, LED_ON);
  delay(250);
  digitalWrite(LED_BUILTIN, LED_OFF);
  delay(250);
}

void setup()
{
  Serial.begin(115200);
  gdbstub_init();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_OFF); // Turn LED off

  WiFi.mode(WIFI_STA);   // Station mode (connect to WiFi network)
  WiFi.persistent(true); // Enable persistent memory

  if (WiFi.SSID().length() > 0)
  {
    // Print the SSID and password
    Serial.println("WiFi credentials stored:");
    Serial.println(WiFi.SSID());
    Serial.println(WiFi.psk());

    Serial.print("Connecting...");
    WiFi.begin(WiFi.SSID(), WiFi.psk());
    WiFi.waitForConnectResult(CONNECTION_TIMEOUT);
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Stored credentials not found. Starting WPS configuration...");
    digitalWrite(LED_BUILTIN, LED_ON); // Indicate to press WPS button
    WiFi.beginWPSConfig();
    digitalWrite(LED_BUILTIN, LED_OFF); // Time to action has passed
    if (WiFi.SSID().length() > 0)
    {
      // Print the SSID and password
      Serial.println("WiFi credentials received via WPS:");
      Serial.println(WiFi.SSID());
      Serial.println(WiFi.psk());

      WiFi.begin(WiFi.SSID(), WiFi.psk());
      WiFi.waitForConnectResult(CONNECTION_TIMEOUT);
    }
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  // Set OTA related parameters
  // Port defaults to 8266
  ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("PCSwitch");

  // No authentication by default
  ArduinoOTA.setPassword("1csqBgpHchquXkzQlgl4");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  // define event callbacks
  ArduinoOTA.onStart([]() {  
    String type;  
    if (ArduinoOTA.getCommand() == U_FLASH) {  
    type = "sketch";  
    } else { // U_SPIFFS  
    type = "filesystem";  
    }  
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()  
    Serial.println("Start updating " + type); });

  ArduinoOTA.onEnd([]() { 
    Serial.println("\nEnd"); });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) { 
    Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });

  ArduinoOTA.onError([](ota_error_t error) {  
    Serial.printf("Error[%u]: ", error);  
    if (error == OTA_AUTH_ERROR) {  
    Serial.println("Auth Failed");  
    } else if (error == OTA_BEGIN_ERROR) {  
    Serial.println("Begin Failed");  
    } else if (error == OTA_CONNECT_ERROR) {  
    Serial.println("Connect Failed");  
    } else if (error == OTA_RECEIVE_ERROR) {  
    Serial.println("Receive Failed");  
    } else if (error == OTA_END_ERROR) {  
    Serial.println("End Failed");  
    } });

  // enable OTA
  ArduinoOTA.begin();
}

void loop()
{
  ArduinoOTA.handle();

  // if (WiFi.status() == WL_CONNECTED) {
  //   Serial.println("Connected");
  //   delay( 1000);
  // }
}
