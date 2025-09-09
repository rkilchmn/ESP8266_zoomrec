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
        
        // Initialize session if not already done
        if (!sslSession) {
            sslSession = std::make_unique<BearSSL::Session>();
        }
        
        // Set the session for the client
        secureClient->setSession(sslSession.get());
    }
    
    if (serverPubKeyPemStr.length() > 0) {
        if (!serverPubKey) {
            serverPubKey = std::unique_ptr<BearSSL::PublicKey>(new BearSSL::PublicKey(serverPubKeyPemStr.c_str()));
        }
        
        if (serverPubKey) {
            secureClient->setKnownKey(serverPubKey.get());
            secureClient->allowSelfSignedCerts();
            secureClient->setBufferSizes(512, 512);
        } else {
            Serial.println(F("ManageWifiClient: Failed to create PublicKey."));
            return;
        }
    } else {
        secureClient->setInsecure();
        Serial.println(F("ManageWifiClient: No public key provided for secure client, using insecure connection."));
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
    return url && strncasecmp(url, "https", 5) == 0;
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
