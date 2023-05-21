#include "RingBuffer.h"

size_t RingBuffer::size() const {
  if (writeIndex >= readIndex) {
    return writeIndex - readIndex;
  } else {
    return (BUFFER_SIZE - readIndex) + writeIndex;
  }
}

size_t RingBuffer::available() const {
  return BUFFER_SIZE - size() - 1;
}

void RingBuffer::push(uint8_t data) {
  size_t nextWriteIndex = (writeIndex + 1) % BUFFER_SIZE;
  if (nextWriteIndex != readIndex) {
    buffer[writeIndex] = data;
    writeIndex = nextWriteIndex;
  }
}

void RingBuffer::push(const uint8_t* data, size_t length) {
  for (size_t i = 0; i < length; i++) {
    push(data[i]);
  }
}

size_t RingBuffer::read(uint8_t* data, size_t length) {
  size_t bytesRead = 0;
  while (bytesRead < length && readIndex != writeIndex) {
    data[bytesRead++] = buffer[readIndex];
    readIndex = (readIndex + 1) % BUFFER_SIZE;
  }
  return bytesRead;
}
