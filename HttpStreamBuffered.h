#ifndef STREAMBUFFERED_H
#define STREAMBUFFERED_H

#include <Arduino.h>
#include "CircularBuffer.hpp"

#include "JSONAPIClient.h"

#define CIRCULAR_BUFFER_SIZE 1024
#define FLUSH_BUFFER_SIZE 256

class HttpStreamBuffered : public Stream
{
protected:
  WiFiClient& client;
  CircularBuffer<uint8_t, CIRCULAR_BUFFER_SIZE> buffer;
  boolean overwriting;
  String logId;
  String url;
  String path; 
  String username;
  String password;
  bool debug;

public:
  HttpStreamBuffered(WiFiClient& client, const char *logId, const char *url, const char *path, const char *http_username, const char *http_password, bool debug = false);
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
