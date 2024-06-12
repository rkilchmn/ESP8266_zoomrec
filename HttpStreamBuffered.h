#ifndef STREAMBUFFERED_H
#define STREAMBUFFERED_H

#include <Arduino.h>
#include "CircularBuffer.hpp"

#include "JSONAPIClient.h"

class HttpStreamBuffered : public Stream
{
protected:
  CircularBuffer<uint8_t, 1024> buffer;
  boolean overwriting;
  const char *logId;
  const char *url;
  const char *path; 
  const char *username;
  const char *password;

public:
  HttpStreamBuffered(const char *logId, const char *url, const char *path, const char *http_username, const char *http_password);
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
};

#endif // STREAMBUFFERED_H
