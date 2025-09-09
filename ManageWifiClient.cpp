#include "ManageWifiClient.h"
#include <Arduino.h>

// Necessary for BearSSL classes
#include <WiFiClientSecure.h>

ManageWifiClient& ManageWifiClient::getInstance() {
    static ManageWifiClient instance;
    return instance;
}

ManageWifiClient::ManageWifiClient() {}

ManageWifiClient::~ManageWifiClient() {}

void ManageWifiClient::initializeSecureClient() {
    if (!secureClient) {
        secureClient = std::unique_ptr<BearSSL::WiFiClientSecure>(new BearSSL::WiFiClientSecure());
    }
    
    if (serverPubKeyPemStr.length() > 0) {
        serverPubKey = std::unique_ptr<BearSSL::PublicKey>(new BearSSL::PublicKey(serverPubKeyPemStr.c_str()));
        if (serverPubKey) {
            secureClient->setKnownKey(serverPubKey.get());
            secureClient->allowSelfSignedCerts();
            secureClient->setBufferSizes(512, 512);
        } else {
            Serial.println(F("ManageWifiClient: Failed to create PublicKey."));
            return;
        }
    } else {
        Serial.println(F("ManageWifiClient: No public key provided for secure client."));
    }
}

void ManageWifiClient::initializeNonSecureClient() {
    if (!nonSecureClient) {
        nonSecureClient = std::unique_ptr<WiFiClient>(new WiFiClient());
    }
}

void ManageWifiClient::init(const char* pubKey) {
    ManageWifiClient& inst = ManageWifiClient::getInstance();
    inst.serverPubKeyPemStr = pubKey ? String(pubKey) : String("");
    // If secure client already exists, reinitialize it with the new key
    if (inst.secureClient) {
        inst.initializeSecureClient();
    }
}

static bool urlStartsWithHttps(const char* url) {
    if (!url) return false;
    // case-insensitive check for "https"
    const char* s = url;
    const char* t = "https";
    for (int i = 0; i < 5; ++i) {
        char c = s[i];
        if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
        if (c != t[i]) return false;
    }
    return true;
}

WiFiClient* ManageWifiClient::getClient(const char* url) {
    ManageWifiClient& inst = ManageWifiClient::getInstance();
    if (urlStartsWithHttps(url)) {
        if (!inst.secureClient) {
            inst.initializeSecureClient();
        }
        return inst.secureClient.get();
    } else {
        inst.initializeNonSecureClient();
        return inst.nonSecureClient.get();
    }
}
