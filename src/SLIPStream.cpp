// This file is part of the SLIPStream library.
// (c) 2018-2019 Shawn Silverman

#include "SLIPStream.h"

// C++ includes (Arduino doesn't have C++ headers, so check)
#if __has_include(<cstring>)
#include <cstring>
#else
#include <string.h>
#endif

// Define SLIP constants.
static constexpr uint8_t END     = 0xc0;  // 0300
static constexpr uint8_t ESC     = 0xdb;  // 0333
static constexpr uint8_t ESC_END = 0xdc;  // 0334
static constexpr uint8_t ESC_ESC = 0xdd;  // 0335

SLIPStream::SLIPStream(Stream &stream)
    : stream_(stream),
      inESC_(false),
      isEND_(false) {}

int SLIPStream::availableForWrite() {
  // Assume each byte takes up two slots
  return stream_.availableForWrite() / 2;
}

size_t SLIPStream::write(const uint8_t *b, size_t size) {
  size_t initialSize = size;
  while (size > 0) {
    if (writeByte(*b) == 0) {
      break;
    }
    size--;
    b++;
  }
  return initialSize - size;
}

size_t SLIPStream::write(uint8_t b) {
  return writeByte(b);
}

size_t SLIPStream::writeEnd() {
  size_t written = stream_.write(END);
  if (written < 1) {
    setWriteError();
    return 0;
  }
  if (stream_.getWriteError()) {
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
  size_t written = 0;
  switch (b) {
    case END:
      written += stream_.write(ESC);
      written += stream_.write(ESC_END);
      break;
    case ESC:
      written += stream_.write(ESC);
      written += stream_.write(ESC_ESC);
      break;
    default:
      if (stream_.write(b) < 1) {
        setWriteError();
        return 0;
      }
      return 1;
  }

  if (written < 2) {
    setWriteError();
    return 0;
  }
  if (stream_.getWriteError() != 0) {
    setWriteError();
  }
  return 1;
}

int SLIPStream::available() {
  int avail = stream_.available();
  if (avail <= 0) {
    return 0;
  }

  int b = peek();
  if (b >= 0 || b == -2) {
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
  switch (b) {
    case ESC:
      if (inESC_) {
        // Protocol violation
        // Choose to return the character
        return b;
      } else {
        return -1;
      }
      break;
    case ESC_END:
      if (inESC_) {
        return END;
      } else {
        return ESC_END;
      }
    case ESC_ESC:
      if (inESC_) {
        return ESC;
      } else {
        return ESC_ESC;
      }
    case END:
      if (inESC_) {
        // Protocol violation
        // Choose to return the character
        return b;
      } else {
        return -2;
      }
    default:
      if (inESC_) {
        // Protocol violation
        // Choose to return the character
      }
      return b;
  }
}

int SLIPStream::read() {
  isEND_ = false;

  while (stream_.available() > 0) {
    int b = stream_.read();
    switch (b) {
      case ESC:
        if (inESC_) {
          // Protocol violation
          // Choose to return the character
          inESC_ = false;
          return b;
        } else {
          inESC_ = true;
        }
        break;
      case ESC_END:
        if (inESC_) {
          inESC_ = false;
          return END;
        } else {
          return ESC_END;
        }
      case ESC_ESC:
        if (inESC_) {
          inESC_ = false;
          return ESC;
        } else {
          return ESC_ESC;
        }
      case END:
        if (inESC_) {
          // Protocol violation
          // Choose to return the character
          inESC_ = false;
          return b;
        } else {
          isEND_ = true;
          return -2;
        }
      default:
        if (inESC_) {
          // Protocol violation
          // Choose to return the character
          inESC_ = false;
        }
        return b;
    }
  }
  return -1;
}
