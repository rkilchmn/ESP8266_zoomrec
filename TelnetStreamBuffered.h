#ifndef TELNETSTREAMBUFFERED_H
#define TELNETSTREAMBUFFERED_H

#include "TelnetStream.h" 
#include "RingBuffer.h" 

class TelnetStreamBuffered : public TelnetStreamClass {
  private:
    RingBuffer buffer;

  public:
    TelnetStreamBuffered(uint16_t port);

    size_t write(uint8_t val) override;
    size_t write(const uint8_t *buf, size_t size) override;
    void flush() override;

  private:
    void flushBufferedData();
};

extern TelnetStreamBuffered BufferedTelenetStream;

#endif // TelnetStreamBuffered_H
