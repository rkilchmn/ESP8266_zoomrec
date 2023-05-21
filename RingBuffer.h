#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <Arduino.h>

class RingBuffer {
private:
  static const size_t BUFFER_SIZE = 1024;
  uint8_t buffer[BUFFER_SIZE];
  size_t readIndex = 0;
  size_t writeIndex = 0;

public:
  size_t size() const;
  size_t available() const;
  void push(uint8_t data);
  void push(const uint8_t* data, size_t length);
  size_t read(uint8_t* data, size_t length);
};

#endif  // RINGBUFFER_H
