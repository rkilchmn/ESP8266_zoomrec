#include "JSONAPIClient.h"

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h> // git clone https://github.com/bblanchon/ArduinoJson.git
#include <base64.h>

// Define a function to send an HTTPS request with basic authentication
int JSONAPIClient::performRequest(
    int method, const char *url, const char *path, 
    JsonDocument& requestHeader, JsonDocument& requestBody, JsonDocument& responseBody,
    const char *http_username, const char *http_password, const char *tls_fingerprint)
{
  WiFiClient client;

  bool debug = false;

  if (strncmp(url, "https", 5) == 0)
  {
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

  (debug) ? Serial.printf("uri=%s", uri.c_str()) : 0;

  // Set the HTTP request headers with the basic authentication credentials
  String auth = String(http_username) + ":" + String(http_password);
  String encodedAuth = base64::encode(auth);
  http.setAuthorization(encodedAuth);

  // Send the HTTP request to the API endpoint
  int httpCode;
  String requestBodyStr;
  int requestBodySize;

  // avoid problem with concurrent calls to http
  // http.addHeader("Connection", "close");
  
  switch (method)
  {
  case HTTP_METHOD_GET:
    httpCode = http.GET();
    break;
  case HTTP_METHOD_POST:
    // Serialize the JSON request body into a string
    requestBodySize = serializeJson(requestBody, requestBodyStr);
   
    if (requestBodySize > 0) {
      // Set the HTTP request headers for a JSON POST request
      http.addHeader("Content-Type", "application/json");
      http.addHeader("Content-Length", String(requestBodyStr.length()));

      (debug) ? Serial.printf("requestBodySize=%d request=%s\n", requestBodySize, requestBodyStr.c_str()) : 0;
      
      // Send the HTTP POST request with the JSON request body
      httpCode = http.POST(requestBodyStr);
    }
    else {
      httpCode = HTTP_CODE_SERIALIZE_REQUESTBODY_FAILED;
    }

    break;

  default:
    // Unsuported HTTP method
    httpCode = HTTP_CODE_UNSUPPORTED_HTTP_METHOD;
  }

  // Read the responseBody JSON data into a DynamicJsonDocument
  if (http.getSize() > 0) {
    DeserializationError error = deserializeJson(responseBody, http.getString());
    if (error)
    {
      httpCode = HTTP_CODE_DESERIALIZE_RESPONSE_FAILED;
    }
  }

  // Release the resources used by the HTTP client
  http.end();

  return httpCode;
}