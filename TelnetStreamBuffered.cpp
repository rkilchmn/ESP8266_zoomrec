#include "TelnetStreamBuffered.h"

TelnetStreamBuffered::TelnetStreamBuffered(uint16_t port) : TelnetStreamClass(port), overwriting(false) {}

size_t TelnetStreamBuffered::write(uint8_t val)
{
  if (disconnected())
  {
    if (!buffer.push(val) && !overwriting)
    {
      overwriting = true;
      Serial.println("TelnetStreamBuffered: buffer is full; now overwriting old data");
    }

    return 1; // we still have stored data
  }
  else
  {
    flushBufferedData();
    return TelnetStreamClass::write(val);
  }
}

size_t TelnetStreamBuffered::write(const uint8_t *buf, size_t size)
{
  if (disconnected())
  {
    bool isFull = false;
    for (size_t i = 0; i < size; i++)
      isFull = !buffer.push(buf[i]);
    if (isFull && !overwriting)
    {
      overwriting = true;
      Serial.println("TelnetStreamBuffered: buffer is full; now overwriting old data");
    }

    return size; // we still have stored data
  }
  else
  {
    flushBufferedData();
    return TelnetStreamClass::write(buf, size);
  }
}

void TelnetStreamBuffered::flush()
{
  if (!disconnected())
  {
    flushBufferedData();
    TelnetStreamClass::flush();
  }
}

void TelnetStreamBuffered::flushBufferedData()
{
  int flushed = 0;
  while (!buffer.isEmpty())
    flushed += TelnetStreamClass::write(buffer.shift());
  if (flushed > 0)
  {
    overwriting = false;
    Serial.printf("TelnetStreamBuffered: flushed buffer %d\n", flushed);
  }
}

TelnetStreamBuffered BufferedTelnetStream(23);
