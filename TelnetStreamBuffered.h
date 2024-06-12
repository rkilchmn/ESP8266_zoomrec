#ifndef TELNETSTREAMBUFFERED_H
#define TELNETSTREAMBUFFERED_H

#include <Arduino.h>
#include <TelnetStream.h>
#include <CircularBuffer.hpp>

class TelnetStreamBuffered : public TelnetStreamClass
{
protected:
  CircularBuffer<uint8_t, 512> buffer;

  boolean overwriting;

public:
  TelnetStreamBuffered(uint16_t port);

  size_t write(uint8_t val);
  size_t write(const uint8_t *buf, size_t size);
  void flush();

private:
  void flushBufferedData();
};

#endif // TELNETSTREAMBUFFERED_H
