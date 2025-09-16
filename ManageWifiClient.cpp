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
    
    if (serverPubKey) {
        secureClient->setKnownKey(serverPubKey.get());
        secureClient->allowSelfSignedCerts();
        secureClient->setBufferSizes(512, 512);
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

void ManageWifiClient::init(const char* serverPubKeyPem) {
    ManageWifiClient& inst = ManageWifiClient::getInstance();
    
    // Clear existing public key
    inst.serverPubKey.reset();
    
    // Create new public key object if a key is provided
    if (serverPubKeyPem && strlen(serverPubKeyPem) > 0) {
        inst.serverPubKey = std::make_unique<BearSSL::PublicKey>(serverPubKeyPem);
    }
    
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
