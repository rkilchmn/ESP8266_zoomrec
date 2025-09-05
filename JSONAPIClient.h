#ifndef HTTPREST_H
#define HTTPREST_H

#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <memory>

class JSONAPIClient {
private:
    static JSONAPIClient* instance;
    std::unique_ptr<WiFiClient> httpClient;
    std::unique_ptr<WiFiClientSecure> httpsClient;
    bool secureInitialized = false;
    bool insecureInitialized = false;
    
    // Private constructor to prevent instantiation
    JSONAPIClient() = default;
    
    // Prevent copying and assignment
    JSONAPIClient(const JSONAPIClient&) = delete;
    JSONAPIClient& operator=(const JSONAPIClient&) = delete;
    
    bool initSecureClient(const char* tls_fingerprint, bool debug);
    bool initInsecureClient(bool debug);
    void cleanup();
    
    // Implementation of the request method
    int performRequestImpl(
        int method, const char *url, const char *path, 
        JsonDocument& requestHeader, JsonDocument& requestBody, JsonDocument& responseBody,
        const char *http_username, const char *http_password, const char *tls_fingerprint
    );

public:
    static const int HTTP_CODE_CONNECTION_FAILED = -1;
    static const int HTTP_CODE_UNSUPPORTED_HTTP_METHOD = -2;
    static const int HTTP_CODE_SERIALIZE_REQUESTBODY_FAILED = -3;
    static const int HTTP_CODE_HTTP_BEGIN_FAILED = -4;
    static const int HTTP_METHOD_GET = 1;
    static const int HTTP_METHOD_POST = 2;

    // Get the singleton instance
    static JSONAPIClient& getInstance();
    
    // Static wrapper for backward compatibility
    static int performRequest(
        int method, const char *url, const char *path, 
        JsonDocument& requestHeader, JsonDocument& requestBody, JsonDocument& responseBody,
        const char *http_username, const char *http_password, const char *tls_fingerprint
    );
    
    ~JSONAPIClient();
};

#endif // HTTPREST_H