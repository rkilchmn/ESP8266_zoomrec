#ifndef STREAMBUFFERED_H
#define STREAMBUFFERED_H

#include <Arduino.h>
#include "CircularBuffer.hpp"

#include "JSONAPIClient.h"

#define CIRCULAR_BUFFER_SIZE 1024

class HttpStreamBuffered : public Stream
{
protected:
  CircularBuffer<uint8_t, CIRCULAR_BUFFER_SIZE> buffer;
  boolean overwriting;
  char *logId;
  char *url;
  char *path; 
  char *username;
  char *password;
  bool debug;

public:
  HttpStreamBuffered(const char *logId, const char *url, const char *path, const char *http_username, const char *http_password, bool debug = false);
  ~HttpStreamBuffered();

  size_t write(uint8_t val);
  size_t write(const uint8_t *buf, size_t size);
  void flush();

   // Stream implementation
  int read();
  int available();
  int peek();

private:
  void flushBufferedData();
  bool callHttpApi( const char *data, long dataSize);
  StaticJsonDocument<0> staticJsonRequestHeader;
  StaticJsonDocument<CIRCULAR_BUFFER_SIZE + 100> staticJsonRequestBody;
  StaticJsonDocument<100> staticJsonResponseBody;
};

#endif // STREAMBUFFERED_H
