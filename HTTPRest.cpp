#include "HTTPRest.h"

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <base64.h>

#include "console.h"

// Define a function to send an HTTPS request with basic authentication
DynamicJsonDocument performHttpsRequest(Console console, const char *method, const char *url, const char *path,
                                        const char *http_username, const char *http_password, const char *tls_fingerprint, size_t response_capacity,
                                        DynamicJsonDocument *requestBody)
{
  WiFiClient client;

  if (strncmp(url, "https", 5) == 0) {
    // Use WiFiClientSecure for HTTPS requests
    WiFiClientSecure secureClient;
  
    if (strlen(tls_fingerprint) > 0)
      secureClient.setFingerprint(tls_fingerprint);
    else
      secureClient.setInsecure();

    client = secureClient;
  }

  // Initialize the HTTP client with the WiFi client and server/port
  HTTPClient http;
  String uri = String(url) + String(path);
  http.begin(client, uri);

  console.log(Console::DEBUG, F("HTTP Rest Request: %s %s"), method, uri.c_str());

  // Set the HTTP request headers with the basic authentication credentials
  String auth = String(http_username) + ":" + String(http_password);
  String encodedAuth = base64::encode(auth);
  http.setAuthorization(encodedAuth);

  // Send the HTTP request to the API endpoint
  int httpCode;
  if (String(method) == "GET")
  {
    httpCode = http.GET();
  }
  else if (String(method) == "POST")
  {
    // Serialize the JSON request body into a string
    String requestBodyStr;
    serializeJson(*requestBody, requestBodyStr);

    // Set the HTTP request headers for a JSON POST request
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Content-Length", String(requestBodyStr.length()));

    // Send the HTTP POST request with the JSON request body
    httpCode = http.POST(requestBodyStr);
  }
  else
  {
    // Invalid HTTP method
    httpCode = -99; // not official code
  }

  DynamicJsonDocument response(response_capacity);

  if (httpCode == HTTP_CODE_OK)
  {
    // Read the response JSON data into a DynamicJsonDocument
    DeserializationError error = deserializeJson(response, http.getString());

    if (error)
    {
      console.log(Console::ERROR, F("Failed to parse response JSON: %s"), error.c_str());
    }
  }
  else
  {
    console.log(Console::ERROR, F("HTTP error: (%d) %s"), httpCode, http.errorToString(httpCode).c_str());
  }

  // Release the resources used by the HTTP client
  http.end();

  return response;
}