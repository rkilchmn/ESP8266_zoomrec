#include "JSONAPIClient.h"

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
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
  if (!http.begin(client, uri)) {
    return HTTP_CODE_HTTP_BEGIN_FAILED;
  }

  (debug) ? Serial.printf("JSONAPIClient::performRequest uri=%s\n", uri.c_str()) : 0;

  // Set the HTTP request headers with the basic authentication credentials
  String auth = String(http_username) + ":" + String(http_password);
  String encodedAuth = base64::encode(auth);
  http.setAuthorization(encodedAuth);

  // Add custom headers from requestHeader JSON document
  for (JsonPair kv : requestHeader.as<JsonObject>()) {
    const char* headerName = kv.key().c_str();
    const char* headerValue = kv.value().as<const char*>();
    if (headerName && headerValue) {
      http.addHeader(headerName, headerValue);
      (debug) ? Serial.printf("JSONAPIClient::performRequest Added header: %s: %s\n", headerName, headerValue) : 0;
    }
  }

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

      (debug) ? Serial.printf("JSONAPIClient::performRequest requestBodySize=%d request=%s\n", requestBodySize, requestBodyStr.c_str()) : 0;
      
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
    (debug) ? Serial.printf("JSONAPIClient::performRequest responseSize=%d responseBody.memoryUsage=%d response=%s\n", http.getSize(), responseBody.memoryUsage(), http.getString().c_str()) : 0;

    if (error)
    {
      // Create a JSON document
      DynamicJsonDocument doc(500);
      doc["message"] = "Error in deserializing response";
      doc["error"] =  error.c_str();
      doc["length"] =  http.getSize();

      // Copy the content of doc to responseBody
      responseBody.set(doc); 
    }
  }
  else {
    (debug) ? Serial.printf("JSONAPIClient::performRequest responseSize=0\n") : 0;
    responseBody.clear();
  }

  // Release the resources used by the HTTP client
  http.end();

  return httpCode;
}