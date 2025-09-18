#include "HttpStreamBuffered.h"
#include "JSONAPIClient.h"
#include <UrlEncode.h>

// Size of the temporary buffer for flushing data
#define FLUSH_BUFFER_SIZE 256

HttpStreamBuffered::HttpStreamBuffered(WiFiClient& client, const char *logId, const char *url, const char *path, 
                                     const char *http_username, const char *http_password, bool debug)
  : client(client), overwriting(false), debug(debug) {
  if (debug) {
    Serial.printf("[HttpStreamBuffered::HttpStreamBuffered] bufferSize=%d\n", CIRCULAR_BUFFER_SIZE);
  }
  // Store strings directly using String class for better memory management
  this->logId = logId;
  this->url = url;
  this->path = path;
  this->username = http_username;
  this->password = http_password;
}

HttpStreamBuffered::~HttpStreamBuffered() {
  // No need to delete anything as we're using String objects now
}

size_t HttpStreamBuffered::write(uint8_t val)
{
  if (buffer.available() < 1) {
    flushBufferedData();
  }

  if (buffer.available() > 0) {
    long written = buffer.push(val);
    return written;
  }
  else {
    return 0;
  }
}

size_t HttpStreamBuffered::write(const uint8_t *buf, size_t size)
{
  if (buffer.available() < size) {
    flushBufferedData();
  }

  if (debug) {
    Serial.printf("[HttpStreamBuffered::write] Writing %d bytes to buffer, available=%d\n", size, buffer.available());
  }

  long written = 0;
  if (buffer.available() >= size) {
    for (size_t i = 0; i < size; i++) {
      written += buffer.push(buf[i]);
    }

    return written;
  }
  return 0; // Return 0 if buffer.available() < size
}

void HttpStreamBuffered::flush()
{
  flushBufferedData();
}

void HttpStreamBuffered::flushBufferedData()
{
  if (buffer.size() == 0) {
    return; // Nothing to do if buffer is empty
  }

  if (debug) {
    Serial.printf("[HttpStreamBuffered] Starting to flush %d bytes to API\n", buffer.size());
  }
  
  // Use a fixed-size buffer to avoid large stack allocations
  char flushBuffer[FLUSH_BUFFER_SIZE + 1]; // +1 for null terminator
  size_t bufferIndex = 0;
  
  // Process the circular buffer in chunks
  while (!buffer.isEmpty()) {
    // Fill the flush buffer up to FLUSH_BUFFER_SIZE or until the circular buffer is empty
    while (bufferIndex < FLUSH_BUFFER_SIZE && !buffer.isEmpty()) {
      flushBuffer[bufferIndex++] = buffer.shift();
    }
    
    // Null-terminate the buffer
    flushBuffer[bufferIndex] = '\0';
    
    // Send the current chunk
    if (!callHttpApi(flushBuffer, bufferIndex)) {
      // On error, we could try to put the data back, but this might cause
      // an infinite loop if the error persists. For now, we'll just drop the data.
      if (debug) {
        Serial.println("[HttpStreamBuffered] Error calling API, dropped data");
      }
      break;
    }
    
    // Reset for next chunk
    bufferIndex = 0;
  }
  
  overwriting = false;
}

bool HttpStreamBuffered::callHttpApi(const char *data, long dataSize) {
  if (dataSize <= 0 || data == nullptr) {
    return false;
  }

  if (debug) {
    Serial.printf("[HttpStreamBuffered::callHttpApi] Calling API with %d bytes\n", dataSize);
  }

  staticJsonRequestBody.clear();
  staticJsonRequestBody["id"] = logId;
  
  // Encode directly without creating a temporary String if possible
  String encoded;
  {
    // Calculate the worst-case encoded size (each character could become %XX)
    size_t maxEncodedSize = (dataSize * 3) + 1; // 3x for %XX encoding, +1 for null terminator
    encoded.reserve(maxEncodedSize);
    encoded = urlEncode(data);
  }
  
  staticJsonRequestBody["content"] = encoded;

  int httpCode = JSONAPIClient::performRequest(
    client,
    JSONAPIClient::HTTP_METHOD_POST,
    url.c_str(),
    path.c_str(),
    staticJsonRequestHeader,
    staticJsonRequestBody,
    staticJsonResponseBody,
    username.c_str(),
    password.c_str()
  );

  if (httpCode != HTTP_CODE_OK) {
    if (debug) {
      Serial.printf("[HttpStreamBuffered::callHttpApi] url='%s' path='%s' httpCode=%d Response:", 
                   url.c_str(), path.c_str(), httpCode);
      serializeJsonPretty(staticJsonResponseBody, Serial);
      Serial.println();
    }
    return false;
  }

  return true;
}

// Stream implementation: no input supported
int HttpStreamBuffered::read() {
  return 0;
}
int HttpStreamBuffered::available() {
  return 0;
}
int HttpStreamBuffered::peek() {
  return 0;
}