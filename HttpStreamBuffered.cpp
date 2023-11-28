#include "HttpStreamBuffered.h"
#include "JSONAPIClient.h"

#include <UrlEncode.h>   // git clone https://github.com/plageoj/urlencode.git


HttpStreamBuffered::HttpStreamBuffered(const char *logId, const char *url, const char *path, const char *http_username, const char *http_password) : overwriting(false) {
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
  if (buffer.available() > 0) {
    long written = buffer.push( val);
    if (!written && !overwriting)
    {
      overwriting = true;
      Serial.println("HttpStreamBuffered: buffer is full; now overwriting old data");
    }
    return written;
  }
  else
  {
    flushBufferedData();
    Serial.println("Recursive HttpStreamBuffered::write(val)");Serial.println("Recursive HttpStreamBuffered::write(val)");
    return HttpStreamBuffered::write(val);
  }
}

size_t HttpStreamBuffered::write(const uint8_t *buf, size_t size)
{
  long written = 0;
  if (buffer.available() >= size) {
    for (size_t i = 0; i < size; i++) {
       written =+ buffer.push( buf[i]);
    }
    return written;
  }
  else {
    flushBufferedData();
    Serial.println("Recursive HttpStreamBuffered::write( buf, size)");
    return HttpStreamBuffered::write( buf, size); // call itself to write - buffer should be flushdatanow
  }
}

void HttpStreamBuffered::flush()
{
  flushBufferedData();
}

void HttpStreamBuffered::flushBufferedData()
{
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
      // push back to buffer (excluding the '\0')
      for ( int i = 0; i < flushDataSize; i++) {
        buffer.push(flushdata[i]);
      }
  }
    else {
      overwriting = false;
    }
  }
}

bool HttpStreamBuffered::callHttpApi( const char *data, long dataSize) {

    // Serial.printf( "httplog: size=%d text='%s'", dataSize, data);
    // return true;

    if (dataSize > 0) {
      DynamicJsonDocument request  = DynamicJsonDocument(dataSize*2);

      request["id"] = logId;
      String encoded = urlEncode(data);
      request["content"] = encoded.c_str();
      request.shrinkToFit();

      DynamicJsonDocument response = JSONAPIClient::performRequest( 
        JSONAPIClient::HTTP_METHOD_POST,  url, path, request,
        username ,password, ""
      );    

      if (response["code"].as<int>() == HTTP_CODE_OK) {
        return true;
      }
      else {
        // print to serial for debugging error
        Serial.println(F("HttpStreamBuffered::callHttpApi Request:"));
        serializeJsonPretty(request, Serial);
        Serial.println(F("\nHttpStreamBuffered::callHttpApi Response:"));
        serializeJsonPretty(response, Serial);
        Serial.println();
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