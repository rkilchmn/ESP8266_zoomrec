#include "JSONRtcMemory.h"

#include <Arduino.h>
#include <ArduinoJson.h> // git clone https://github.com/bblanchon/ArduinoJson.git
#include <fastlz.h> // git clone https://github.com/ariya/FastLZ.git

bool JSONRtcMemory::save(DynamicJsonDocument& jsonDoc, size_t rtcMemorySize) {
    // Serialize JSON to a string
    String jsonString;
    serializeJson(jsonDoc, jsonString);

    // Compress the serialized JSON data using FastLZ
    uint8_t compressedData[rtcMemorySize]; // Local compressedData
    int compressedSize = fastlz_compress_level(1, jsonString.c_str(), jsonString.length(), compressedData);

    // Convert compressedData to uint32_t array
    uint32_t compressedDataUint32[rtcMemorySize / 4];
    memcpy(compressedDataUint32, compressedData, compressedSize);

    // Store the compressed data in RTC memory
    if (compressedSize > 0 && compressedSize <= rtcMemorySize) {
      ESP.rtcUserMemoryWrite(0, compressedDataUint32, (compressedSize + 3) / 4); // Calculate the number of uint32_t elements
      return true;
    }

    return false;
  }

  // Load JSON document from RTC memory with decompression
bool JSONRtcMemory::load(DynamicJsonDocument& jsonDoc, size_t rtcMemorySize) {
    uint32_t compressedDataUint32[rtcMemorySize / 4]; // Temporary array of uint32_t
    int compressedSizeUint32 = ESP.rtcUserMemoryRead(0, compressedDataUint32, (rtcMemorySize + 3) / 4); // Calculate the number of uint32_t elements
    int compressedSize = compressedSizeUint32 * 4;

    if (compressedSize > 0) {
      // Convert uint32_t array back to uint8_t
      uint8_t compressedData[rtcMemorySize];
      memcpy(compressedData, compressedDataUint32, compressedSize);

      uint8_t decompressedData[rtcMemorySize];
      int decompressedSize = fastlz_decompress(compressedData, compressedSize, decompressedData, rtcMemorySize);

      if (decompressedSize > 0) {
        char loadedJsonData[decompressedSize + 1]; // +1 for null-termination
        memcpy(loadedJsonData, decompressedData, decompressedSize);
        loadedJsonData[decompressedSize] = '\0'; // Null-terminate the string

        DeserializationError error = deserializeJson(jsonDoc, loadedJsonData);
        jsonDoc.shrinkToFit();
        return !error;
      }
    }

    return false;
  }