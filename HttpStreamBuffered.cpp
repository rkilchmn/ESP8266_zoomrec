#include "HttpStreamBuffered.h"
#include "JSONAPIClient.h"

#include <UrlEncode.h>


HttpStreamBuffered::HttpStreamBuffered(const char *logId, const char *url, const char *path, 
                                     const char *http_username, const char *http_password, bool debug)
  : overwriting(false), debug(debug) {
  if (debug) {
    Serial.printf("[HttpStreamBuffered::HttpStreamBuffered] bufferSize=%d\n", CIRCULAR_BUFFER_SIZE);
  }
  this->logId = new char[strlen(logId) + 1];
  this->url = new char[strlen(url) + 1];
  this->path = new char[strlen(path) + 1];
  this->username = new char[strlen(http_username) + 1];
  this->password = new char[strlen(http_password) + 1];

  // Use strncpy to copy strings and ensure null termination
  strncpy(this->logId, logId, strlen(logId));
  this->logId[strlen(logId)] = '\0';

  strncpy(this->url, url, strlen(url));
  this->url[strlen(url)] = '\0';

  strncpy(this->path, path, strlen(path));
  this->path[strlen(path)] = '\0';

  strncpy(this->username, http_username, strlen(http_username));
  this->username[strlen(http_username)] = '\0';

  strncpy(this->password, http_password, strlen(http_password));
  this->password[strlen(http_password)] = '\0';
}

HttpStreamBuffered::~HttpStreamBuffered() {
  delete[] logId;
  delete[] url;
  delete[] path;
  delete[] username;
  delete[] password;
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
  if (debug) {
    Serial.printf("[HttpStreamBuffered] Starting to flush %d bytes to API\n", buffer.size());
  }
  
  if (buffer.size() > 0) {
    long flushDataSize = buffer.size() + 1;
    char flushdata[flushDataSize]; // Create a char array to store the data
    int charArrayIndex = 0; // Initialize an index for the char array

    while (!buffer.isEmpty()) {
      char nextChar = buffer.shift();
      if (charArrayIndex < flushDataSize - 1) {
          flushdata[charArrayIndex] = nextChar; // Store the character in the char array
          charArrayIndex++; // Increment the index
      }
    }
    
    flushdata[charArrayIndex] = '\0'; // Null-terminate the char array
    flushDataSize = charArrayIndex;

    if (!callHttpApi(flushdata, flushDataSize)) { // on error
      // insert back to buffer (excluding the '\0')
      // for ( int i = 0; i < flushDataSize; i++) {
      //   buffer.unshift(flushdata[i]);
      // }
  }
    else {
      overwriting = false;
    }
  }
}

bool HttpStreamBuffered::callHttpApi( const char *data, long dataSize) {

    if (debug) {
      Serial.printf("[HttpStreamBuffered::callHttpApi] Calling API with %d bytes\n", dataSize);
    }

    if (dataSize > 0) {
      staticJsonRequestBody.clear();

      staticJsonRequestBody["id"] = logId;
      String encoded = urlEncode(data);
      staticJsonRequestBody["content"] = encoded.c_str();

      int httpCode = JSONAPIClient::performRequest( 
        JSONAPIClient::HTTP_METHOD_POST, url, path, 
        staticJsonRequestHeader, staticJsonRequestBody, staticJsonResponseBody, 
        username ,password, ""
      );    

      if (httpCode == HTTP_CODE_OK) {
        return true;
      }
      else {
        if (debug) {
          Serial.printf("[HttpStreamBuffered::callHttpApi] url='%s' path='%s' httpCode=%d Response:", url, path, httpCode);
          serializeJsonPretty(staticJsonResponseBody, Serial);
          Serial.println();
        }
        return false;
      }
    }
  else {
    return false;
  }
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