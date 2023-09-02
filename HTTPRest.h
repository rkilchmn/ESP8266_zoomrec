#ifndef HTTPREST_H
#define HTTPREST_H

#include <ArduinoJson.h>
#include "Console.h"

DynamicJsonDocument performHttpsRequest(Console console, const char *method, const char *url, const char *path,
                                        const char *http_username, const char *http_password, const char *tls_fingerprint, size_t response_capacity = 1024,
                                        DynamicJsonDocument *requestBody = nullptr);


#endif // HTTPREST_H