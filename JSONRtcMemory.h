#ifndef JSONRTCMEMORY_H
#define JSONRTCMEMORY_H

#include <ArduinoJson.h> // git clone https://github.com/bblanchon/ArduinoJson.git

class JSONRtcMemory {
public:
  // Save a JSON document to RTC memory with compression
  static bool save(DynamicJsonDocument& jsonDoc, size_t rtcMemorySize = 512);

  // Load JSON document from RTC memory with decompression
  static bool load(DynamicJsonDocument& jsonDoc, size_t rtcMemorySize = 512);
};

#endif  // JSONRTCMEMORY_H
