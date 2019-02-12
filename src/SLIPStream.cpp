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

SLIPStream::SLIPStream(Stream &stream, size_t writeBufSize)
    : stream_(stream),
      bufIndex_(0),
      inESC_(false) {
  if (writeBufSize < 2) {
    writeBufSize = 2;
  }
  bufSize_ = writeBufSize;
  buf_ = new uint8_t[writeBufSize];
}

SLIPStream::~SLIPStream() {
  if (buf_ != nullptr) {
    delete[] buf_;
  }
}

int SLIPStream::availableForWrite() {
  // Assume each byte takes up two slots in the buffer
  return (bufSize_ - bufIndex_)/2;
}

size_t SLIPStream::write(const uint8_t *b, size_t size) {
  if (getWriteError() != 0) {
    return 0;
  }

  size_t initialSize = size;
  while (size > 0) {
    if (!writeByte(*b)) {
      break;
    }
    size--;
    b++;
  }
  return initialSize - size;
}

size_t SLIPStream::write(uint8_t b) {
  if (getWriteError() != 0) {
    return 0;
  }
  return writeByte(b) ? 1 : 0;
}

size_t SLIPStream::writeEnd() {
  if (getWriteError() != 0) {
    return 0;
  }

  // Write any remaining characters first
  if (bufIndex_ >= bufSize_) {
    if (!writeBuf()) {
      return 0;
    }
  }
  buf_[bufIndex_++] = END;
  return 1;
}

void SLIPStream::flush() {
  if (getWriteError() != 0) {
    return;
  }
  if (writeBuf()) {
    stream_.flush();
  }
}

bool SLIPStream::writeBuf() {
  if (bufIndex_ == 0) {
    return true;
  }

  // Write as many bytes as we can
  const uint8_t *b = buf_;
  size_t size = bufIndex_;
  while (size > 0) {
    size_t written = stream_.write(b, size);

    // Use any existing error
    int writeErr = stream_.getWriteError();
    if (writeErr != 0) {
      // Set a write error because this was a short write, but keep any write
      // error that was set
      setWriteError(writeErr);
      break;
    }

    // Avoid an infinite loop and treat all short writes as an error
    if (written < 1) {
      setWriteError();
      break;
    }

    size -= written;
    b += written;
  }

  size_t written = bufIndex_ - size;

  if (written >= bufIndex_) {
    bufIndex_ = 0;
    return true;
  }

  // If we've written nothing then no need to shift the buffer
  if (written > 0) {
    size_t diff = bufIndex_ - written;
    memmove(buf_, &buf_[written], diff);
    bufIndex_ = diff;
  }
  return false;
}

// Writes a byte to the buffer.
bool SLIPStream::writeByte(uint8_t b) {
  switch (b) {
    case END:
      if (bufIndex_ >= bufSize_ - 1) {
        if (!writeBuf()) {
          return false;
        }
      }
      buf_[bufIndex_++] = ESC;
      buf_[bufIndex_++] = ESC_END;
      break;
    case ESC:
      if (bufIndex_ >= bufSize_ - 1) {
        if (!writeBuf()) {
          return false;
        }
      }
      buf_[bufIndex_++] = ESC;
      buf_[bufIndex_++] = ESC_ESC;
      break;
    default:
      if (bufIndex_ >= bufSize_) {
        if (!writeBuf()) {
          return false;
        }
      }
      buf_[bufIndex_++] = b;
  }
  return true;
}

int SLIPStream::available() {
  if (stream_.available() <= 0) {
    return 0;
  }
  if (peek() >= 0) {
    return 1;
  }
  return 0;
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
