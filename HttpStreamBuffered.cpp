#include "HttpStreamBuffered.h"
#include "JSONAPIClient.h"

HttpStreamBuffered::HttpStreamBuffered(const char *logId, const char *url, const char *path, const char *http_username, const char *http_password) : overwriting(false) {
  logId = new char[strlen(logId)];
  url = new char[strlen(url)];
  path = new char[strlen(path)];
  username = new char[strlen(http_username)];
  password = new char[strlen(http_password)];
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
    return HttpStreamBuffered::write( buf, size); // call itself to write - buffer should be flushdatanow
  }
}

void HttpStreamBuffered::flush()
{
  flushBufferedData();
}

void HttpStreamBuffered::flushBufferedData()
{
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
    for (int i = 0; i < flushDataSize-1; i++) {
      buffer.push(flushdata[i]);
    }
  }
  else {
    overwriting = false;
  }
}

bool HttpStreamBuffered::callHttpApi( const char *data, long dataSize) {

    Serial.printf( "httplog: size=%d text='%s'", dataSize, data);
    return true;

    // DynamicJsonDocument request  = DynamicJsonDocument(200);

    // request["id"] = logId;
    // request["content"] = data;
    // request.shrinkToFit();

    // DynamicJsonDocument response = JSONAPIClient::performRequest( 
    //   JSONAPIClient::HTTP_METHOD_POST,  url, path, request,
    //   username ,password, ""
    // );    

    // if (response["code"].as<int>() == HTTP_CODE_OK) {
    //   return true;
    // }
    // else {
    //   // print to serial for debugging error
    //   serializeJsonPretty(response, Serial);
    //   Serial.println();
    //   return false;
    // }
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