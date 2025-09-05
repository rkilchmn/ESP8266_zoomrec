#include "JSONAPIClient.h"
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <base64.h>

// Initialize static member
JSONAPIClient* JSONAPIClient::instance = nullptr;

// Singleton instance getter
JSONAPIClient& JSONAPIClient::getInstance() {
    if (!instance) {
        instance = new JSONAPIClient();
    }
    return *instance;
}

JSONAPIClient::~JSONAPIClient() {
    cleanup();
    instance = nullptr;
}

void JSONAPIClient::cleanup() {
    if (httpClient) {
        httpClient->stop();
        httpClient.reset();
        insecureInitialized = false;
    }
    if (httpsClient) {
        httpsClient->stop();
        httpsClient.reset();
        secureInitialized = false;
    }
}

bool JSONAPIClient::initSecureClient(const char* tls_fingerprint, bool debug) {
    if (!secureInitialized) {
        httpsClient = std::make_unique<WiFiClientSecure>();
        
        // Set buffer sizes to 512 bytes each for secure client
        httpsClient->setBufferSizes(512, 512);
        
        if (strlen(tls_fingerprint) > 0) {
            httpsClient->setFingerprint(tls_fingerprint);
            if (debug) {
                Serial.println("JSONAPIClient: Using HTTPS with TLS fingerprint verification");
                Serial.printf("Fingerprint: %s\n", tls_fingerprint);
            }
        } else {
            httpsClient->setInsecure();
            if (debug) {
                Serial.println("JSONAPIClient: WARNING! Using HTTPS without certificate validation (INSECURE)");
            }
        }
        secureInitialized = true;
    }
    return secureInitialized;
}

bool JSONAPIClient::initInsecureClient(bool debug) {
    if (!insecureInitialized) {
        httpClient = std::make_unique<WiFiClient>();
        if (debug) {
            Serial.println("JSONAPIClient: Using plain HTTP (no encryption)");
        }
        insecureInitialized = true;
    }
    return insecureInitialized;
}

int JSONAPIClient::performRequest(
    int method, const char *url, const char *path, 
    JsonDocument& requestHeader, JsonDocument& requestBody, JsonDocument& responseBody,
    const char *http_username, const char *http_password, const char *tls_fingerprint)
{
    return getInstance().performRequestImpl(
        method, url, path, requestHeader, requestBody, responseBody,
        http_username, http_password, tls_fingerprint
    );
}

int JSONAPIClient::performRequestImpl(
    int method, const char *url, const char *path, 
    JsonDocument& requestHeader, JsonDocument& requestBody, JsonDocument& responseBody,
    const char *http_username, const char *http_password, const char *tls_fingerprint)
{
    bool debug = false;
    bool isHttps = strncmp(url, "https", 5) == 0;
    
    // Initialize the appropriate client
    bool clientReady = isHttps ? 
        initSecureClient(tls_fingerprint, debug) : 
        initInsecureClient(debug);
        
    if (!clientReady) {
        return HTTP_CODE_HTTP_BEGIN_FAILED;
    }
    
    // Get the appropriate client
    WiFiClient* client = isHttps ? httpsClient.get() : httpClient.get();

    // Initialize the HTTP client with the WiFi client and server/port
    HTTPClient http;
    String uri = String(url) + String(path);
    if (!http.begin(*client, uri)) {
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

  // Clean up if there was an error to ensure fresh connection next time
  if (httpCode <= 0) {
      cleanup();
  }

  return httpCode;
}