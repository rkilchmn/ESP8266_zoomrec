#include <TelnetStream.h>
#include "RingBuffer.h"

class TelnetStreamBuffered : public TelnetStreamClass {
  protected:
    RingBuffer buffer;

  public:
    TelnetStreamBuffered(uint16_t port) : TelnetStreamClass(port) {}

    size_t write(uint8_t val) {
      if (disconnected()) {
        buffer.push(val);
        return 1;
      }
      flushBufferedData();
      return TelnetStreamClass::write(val);
    }

    size_t write(const uint8_t *buf, size_t size) {
      if (disconnected()) {
        buffer.push(buf, size);
        return size;
      }
      flushBufferedData();
      return TelnetStreamClass::write(buf, size);
    }

    void flush() {
      flushBufferedData();
      TelnetStreamClass::flush();
    }

  private:
    void flushBufferedData() {
      size_t bufferSize = buffer.size();
      if (bufferSize > 0) {
        uint8_t *data = new uint8_t[bufferSize];
        size_t bytesRead = buffer.read(data, bufferSize);
        TelnetStreamClass::write(data, bytesRead);
        delete[] data;
      }
    }
};

TelnetStreamBuffered BufferedTelenetStream(23);
