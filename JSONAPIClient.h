#ifndef HTTPREST_H
#define HTTPREST_H

#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

class JSONAPIClient {
public:   
    static const int HTTP_CODE_HTTP_BEGIN_FAILED = -1;
    static const int HTTP_CODE_CONNECTION_FAILED = -2;
    static const int HTTP_CODE_UNSUPPORTED_HTTP_METHOD = -3;
    static const int HTTP_CODE_SERIALIZE_REQUESTBODY_FAILED = -4;
    static const int HTTP_CODE_DESERIALIZE_RESPONSEBODY_FAILED = -5;
    
    static const int HTTP_METHOD_GET = 1;
    static const int HTTP_METHOD_POST = 2;

    // Main request method
    static int performRequest(
        WiFiClient& client, 
        int method, 
        const char *url, 
        const char *path, 
        JsonDocument& requestHeader, 
        JsonDocument& requestBody, 
        JsonDocument& responseBody,
        const char *http_username = nullptr, 
        const char *http_password = nullptr
    );
};

#endif // HTTPREST_H