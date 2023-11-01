#include "HttpRestStreamBuffered.h"

HttpRestStreamBuffered::HttpRestStreamBuffered(const char *logId, const char *url, const char *path, const char *http_username, const char *http_password) : overwriting(false) {
  logId = new char[strlen(logId)];
  httpRestUrl = new char[strlen(url)];
  httpRestPath = new char[strlen(path)];
  httpRestUsername = new char[strlen(http_username)];
  httpRestPassword = new char[strlen(http_password)];
}

HttpRestStreamBuffered::~HttpRestStreamBuffered() {
  delete[] logId;
  delete[] httpRestUrl;
  delete[] httpRestPath;
  delete[] httpRestUsername;
  delete[] httpRestPassword;
}
size_t HttpRestStreamBuffered::write(uint8_t val)
{
  if (buffer.available() > 0) {
    long written = buffer.push( val);
    if (!written && !overwriting)
    {
      overwriting = true;
      Serial.println("HttpRestStreamBuffered: buffer is full; now overwriting old data");
    }
    return written;
  }
  else
  {
    flushBufferedData();
    return write(val);
  }
}

size_t HttpRestStreamBuffered::write(const uint8_t *buf, size_t size)
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
    return write( buf, size); // call itself to write - buffer should be flushdatanow
  }
}

void HttpRestStreamBuffered::flush()
{
  flushBufferedData();
}

void HttpRestStreamBuffered::flushBufferedData()
{
  long flushDataSize = buffer.size() + 1;
  char flushdata[flushDataSize]; // Create a char array to store the data
  int charArrayIndex = 0; // Initialize an index for the char array

  while (!buffer.isEmpty()) {
    char nextChar = buffer.shift();
    if (charArrayIndex < flushDataSize - 1) {
        flushdata[charArrayIndex] = nextChar; // Store the character in the char array
        charArrayIndex++; // Increment the index
        flushdata[charArrayIndex] = '\0'; // Null-terminate the char array
    }

    if (!callHttpRest(flushdata, flushDataSize)) { // on error
      // push back to buffer (excluding the '\0')
      for (int i = 0; i < charArrayIndex-1; i++) {
        buffer.push(flushdata[i]);
      }
    }
  }
}

bool HttpRestStreamBuffered::callHttpRest( const char *data, long dataSize) {
  // DynamicJsonDocument request  = DynamicJsonDocument(dataSize + 20);

  // request["id"] = logId;
  // request["content"] = String( data);
  // request.shrinkToFit();

  // DynamicJsonDocument response = performHttpsRequest( "GET", httpRestUrl, httpRestPath, request,
  //                                                     httpRestUsername, httpRestPassword, "");                                           

  // return !response.isNull();
  return true;
}

// Stream implementation: no input supported
int HttpRestStreamBuffered::read() {
  return 0;
}
int HttpRestStreamBuffered::available() {
  return 0;
}
int HttpRestStreamBuffered::peek() {
  return 0;
}