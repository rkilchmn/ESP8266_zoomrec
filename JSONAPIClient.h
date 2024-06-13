#ifndef HTTPREST_H
#define HTTPREST_H

#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

class JSONAPIClient {
public:
    static const int HTTP_CODE_CONNECTION_FAILED = -1;
    static const int HTTP_CODE_UNSUPPORTED_HTTP_METHOD = -2;
    static const int HTTP_CODE_DESERIALIZE_RESPONSE_FAILED = -3;
    static const int HTTP_CODE_SERIALIZE_REQUESTBODY_FAILED = -4;
    static const int HTTP_CODE_HTTP_BEGIN_FAILED = -5;
    static const int HTTP_METHOD_GET = 1;
    static const int HTTP_METHOD_POST = 2;

    static int performRequest (
        int method, const char *url, const char *path, 
        JsonDocument& requestHeader, JsonDocument& requestBody, JsonDocument& responseBody,
        const char *http_username, const char *http_password, const char *tls_fingerprint
    );
};

#endif // HTTPREST_H