#include <Arduino.h>
#include <ArduinoJson.h> // git clone https://github.com/bblanchon/ArduinoJson.git
#include <fastlz.h> // git clone https://github.com/ariya/FastLZ.git

class RTCJsonCompressedStorage {
public:
  // Save a JSON document to RTC memory with compression
  static bool save(JsonObject& jsonDoc, size_t rtcMemorySize = 512) {
    // Serialize JSON to a string
    String jsonString;
    serializeJson(jsonDoc, jsonString);

    // Compress the serialized JSON data using FastLZ
    uint8_t compressedData[rtcMemorySize];
    int compressedSize = fastlz_compress_level(1, jsonString.c_str(), jsonString.length(), compressedData);

    // Store the compressed data in RTC memory
    if (compressedSize > 0 && compressedSize <= rtcMemorySize) {
      ESP.rtcUserMemoryWrite(0, compressedData, compressedSize);
      return true;
    }

    return false;
  }

  // Load JSON document from RTC memory with decompression
  static bool load(JsonObject& jsonDoc, size_t rtcMemorySize = 512) {
    uint8_t compressedData[rtcMemorySize];
    int compressedSize = ESP.rtcUserMemoryRead(0, compressedData, rtcMemorySize);

    if (compressedSize > 0) {
      uint8_t decompressedData[rtcMemorySize];
      int decompressedSize = fastlz_decompress(compressedData, compressedSize, decompressedData, rtcMemorySize);

      if (decompressedSize > 0) {
        String loadedJsonData((const char*)decompressedData, decompressedSize);
        
        DeserializationError error = deserializeJson(jsonDoc, loadedJsonData);
        return !error;
      }
    }

    return false;
  }
};
