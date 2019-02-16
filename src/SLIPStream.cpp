// This file is part of the SLIPStream library.
// (c) 2018-2019 Shawn Silverman

#include "SLIPStream.h"

// Define SLIP constants.
static constexpr uint8_t END     = 0xc0;  // 0300
static constexpr uint8_t ESC     = 0xdb;  // 0333
static constexpr uint8_t ESC_END = 0xdc;  // 0334
static constexpr uint8_t ESC_ESC = 0xdd;  // 0335

SLIPStream::SLIPStream(Stream &stream)
    : stream_(stream),
      inESC_(false),
      isEND_(false),
      isBadData_(false) {}

int SLIPStream::availableForWrite() {
  // Assume each byte takes up two slots
  return stream_.availableForWrite() / 2;
}

size_t SLIPStream::write(const uint8_t *buf, size_t size) {
  size_t startSize = size;
  while (size > 0) {
    if (writeByte(*buf) == 0) {
      setWriteError();
      break;
    }
    buf++;
    size--;
  }
  if (stream_.getWriteError() != 0) {
    setWriteError();
  }
  return startSize - size;
}

size_t SLIPStream::write(uint8_t b) {
  if (writeByte(b) == 0) {
    setWriteError();
    return 0;
  }
  if (stream_.getWriteError() != 0) {
    setWriteError();
  }
  return 1;
}

size_t SLIPStream::writeEnd() {
  if (stream_.write(END) == 0) {
    setWriteError();
    return 0;
  }
  if (stream_.getWriteError() != 0) {
    setWriteError();
  }
  return 1;
}

void SLIPStream::flush() {
  stream_.flush();
  if (stream_.getWriteError() != 0) {
    setWriteError();
  }
}

// Writes a byte to the buffer.
size_t SLIPStream::writeByte(uint8_t b) {
  switch (b) {
    case END:
      b = ESC_END;
      break;
    case ESC:
      b = ESC_ESC;
      break;
    default:
      if (stream_.write(b) == 0) {
        setWriteError();
        return 0;
      }
      return 1;
  }

  if (stream_.write(ESC) == 0) {
    setWriteError();
    return 0;
  }
  if (stream_.write(b) == 0) {
    setWriteError();
    return 0;
  }
  return 1;
}

int SLIPStream::available() {
  int avail = stream_.available();
  if (avail <= 0) {
    return 0;
  }

  int b = peek();
  if (b >= 0 || b == END_FRAME) {
    // Guaranteed 1 plus half remaining
    return 1 + (avail - 1)/2;
  }
  return avail/2;
}

int SLIPStream::peek() {
  int b = stream_.peek();
  if (b < 0) {
    return b;
  }
  if (inESC_) {
    switch (b) {
      case ESC_END:
        return END;
      case ESC_ESC:
        return ESC;
      default:
        // Protocol violation
        return b;
    }
  }
  switch (b) {
    case ESC:
      return -1;
    case END:
      return END_FRAME;
    default:
      return b;
  }
}

int SLIPStream::read() {
  isEND_ = false;
  isBadData_ = false;

  while (stream_.available() > 0) {
    int b = stream_.read();
    if (inESC_) {
      inESC_ = false;
      switch (b) {
        case ESC_END:
          return END;
        case ESC_ESC:
          return ESC;
        default:
          // Protocol violation
          isBadData_ = true;
          return b;
      }
    }
    switch (b) {
      case ESC:
        inESC_ = true;
        break;
      case END:
        isEND_ = true;
        return END_FRAME;
      default:
        return b;
    }
  }
  return -1;
}

size_t SLIPStream::read(uint8_t *buf, size_t len) {
  size_t startLen = len;
  while (len > 0) {
    int c = read();
    if (c < 0) {
      break;
    }
    *(buf++) = c;
    len--;
    if (isBadData_) {
      break;
    }
  }
  return startLen - len;
}
