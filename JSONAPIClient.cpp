#include "JSONAPIClient.h"
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#include <base64.h>

int JSONAPIClient::performRequest(
    WiFiClient& client, 
    int method, 
    const char *url, 
    const char *path,
    JsonDocument& requestHeader, 
    JsonDocument& requestBody, 
    JsonDocument& responseBody,
    const char *http_username, 
    const char *http_password) 
{
    bool debug = false;
    HTTPClient http;
    
    // Build the full URL
    String fullUrl = String(url) + String(path);
    
    if (debug) {
        Serial.printf("JSONAPIClient::performRequest uri=%s\n", fullUrl.c_str());
    }

    // Use the provided client
    if (!http.begin(client, fullUrl)) {
        if (debug) {
            Serial.println("JSONAPIClient: Failed to begin HTTP connection");
        }
        return HTTP_CODE_HTTP_BEGIN_FAILED;
    }
    
    // Set basic auth if credentials provided
    if (http_username && http_password && strlen(http_username) > 0) {
        String auth = String(http_username) + ":" + String(http_password);
        String encodedAuth = base64::encode(auth);
        http.setAuthorization(encodedAuth);
    }
    
    // Add custom headers from requestHeader JSON document
    for (JsonPair kv : requestHeader.as<JsonObject>()) {
        const char* headerName = kv.key().c_str();
        const char* headerValue = kv.value().as<const char*>();
        if (headerName && headerValue) {
            http.addHeader(headerName, headerValue);
            if (debug) {
                Serial.printf("JSONAPIClient:: Added header: %s: %s\n", headerName, headerValue);
            }
        }
    }
    
    // Set content type header
    http.addHeader("Content-Type", "application/json");
    // Add keep-alive header to maintain persistent connection
    http.addHeader("Connection", "keep-alive");
    
    int httpCode = 0;
    String requestBodyStr;
    
    // Send the request
    switch (method) {
        case HTTP_METHOD_GET:
            httpCode = http.GET();
            break;
            
        case HTTP_METHOD_POST: {
            int requestBodySize = serializeJson(requestBody, requestBodyStr);
            if (requestBodySize > 0) {
                http.addHeader("Content-Length", String(requestBodyStr.length()));
                if (debug) {
                    Serial.printf("JSONAPIClient:: Sending request body: %s\n", requestBodyStr.c_str());
                }
                httpCode = http.POST(requestBodyStr);
            } else {
                httpCode = HTTP_CODE_SERIALIZE_REQUESTBODY_FAILED;
            }
            break;
        }
            
        default:
            if (debug) {
                Serial.printf("JSONAPIClient: Unsupported HTTP method: %d\n", method);
            }
            http.end();
            return HTTP_CODE_UNSUPPORTED_HTTP_METHOD;
    }
    
    // Handle the response
    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            String payload = http.getString();
            if (debug) {
                Serial.printf("JSONAPIClient:: Received response: %s\n", payload.c_str());
            }
            DeserializationError error = deserializeJson(responseBody, payload);
            if (error) {
                if (debug) {
                    Serial.print("JSONAPIClient: Failed to parse JSON: ");
                    Serial.println(error.c_str());
                }
                http.end();
                return HTTP_CODE_CONNECTION_FAILED;
            }
        }
    } else {
        if (debug) {
            Serial.printf("JSONAPIClient: HTTP request failed, error: %s\n", 
                         http.errorToString(httpCode).c_str());
        }
        http.end();
        return HTTP_CODE_CONNECTION_FAILED;
    }
    
    http.end();
    return httpCode;
}