#ifndef MANAGEWIFICLIENT_H
#define MANAGEWIFICLIENT_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <memory>

// Forward declaration for BearSSL::PublicKey
namespace BearSSL {
    class PublicKey;
}

class ManageWifiClient {
public:
    static ManageWifiClient& getInstance();
    static void init(const char* serverPubKeyPem);
    static WiFiClient* getClient(const char* url);

private:
    ManageWifiClient();
    ~ManageWifiClient();
    ManageWifiClient(const ManageWifiClient&) = delete;
    void operator=(const ManageWifiClient&) = delete;

    String serverPubKeyPemStr; // own the PEM contents to ensure lifetime safety
    std::unique_ptr<WiFiClient> nonSecureClient;
    std::unique_ptr<BearSSL::WiFiClientSecure> secureClient;
    std::unique_ptr<BearSSL::PublicKey> serverPubKey;

    void initializeSecureClient();
    void initializeNonSecureClient();
};

#endif // MANAGEWIFICLIENT_H
