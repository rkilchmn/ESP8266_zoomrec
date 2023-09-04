#ifndef TELNETSTREAMBUFFERED_H
#define TELNETSTREAMBUFFERED_H

#include <Arduino.h>
#include <TelnetStream.h> // git clone https://github.com/JAndrassy/TelnetStream.git
#include <CircularBuffer.h> // git clone https://github.com/JAndrassy/TelnetStream.git

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
