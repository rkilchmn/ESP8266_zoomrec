#ifndef HTTPRESTSTREAMBUFFERED_H
#define HTTPRESTSTREAMBUFFERED_H

#include <Arduino.h>
#include <CircularBuffer.h> // git clone https://github.com/JAndrassy/TelnetStream.git

#include "JSONAPIClient.h"

class HttpRestStreamBuffered : public Stream
{
protected:
  CircularBuffer<uint8_t, 1024> buffer;
  boolean overwriting;
  const char *logId;
  const char *httpRestUrl;
  const char *httpRestPath; 
  const char *httpRestUsername;
  const char *httpRestPassword;

public:
  HttpRestStreamBuffered(const char *logId, const char *url, const char *path, const char *http_username, const char *http_password);
  ~HttpRestStreamBuffered();

  size_t write(uint8_t val);
  size_t write(const uint8_t *buf, size_t size);
  void flush();

   // Stream implementation
  int read();
  int available();
  int peek();

private:
  void flushBufferedData();
  bool callHttpRest( const char *data, long dataSize);
};

#endif // HTTPRESTSTREAMBUFFERED_H
