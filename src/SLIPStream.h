// Library for reading and writing SLIP using an underlying Stream object.
// This file is part of the SLIPStream library.
// (c) 2018-2019 Shawn Silverman

#ifndef SLIPSTREAM_H_
#define SLIPSTREAM_H_

// C++ includes (Arduino doesn't have C++ headers, so check)
#if __has_include(<cstddef>)
#include <cstddef>
#else
#include <stddef.h>
#endif

// Other includes
#include <Stream.h>

class SLIPStream : public Stream {
 public:
  // Creates a new SLIPStream object and uses the given write buffer size.
  //
  // The write buffer size should ideally be twice the number of desired
  // characters because there's a chance, depending on value, that one byte is
  // written out as two bytes to the underlying stream. If it is specified as
  // less than two bytes then it will be set to two bytes.
  SLIPStream(Stream &stream, size_t writeBufSize);

  virtual ~SLIPStream();

  // Returns how many bytes can be written without blocking. Internally, this
  // assumes that each byte will be written as two, so this returns half the
  // remaining buffer size.
  int availableForWrite() override;

  // Writes a range of bytes. This may not write all the bytes if there was a
  // write error or if the underlying stream's write call returns zero. If this
  // is the case then a write error will be set. This will return, however, the
  // actual number of bytes written.
  //
  // This returns zero if there is already a write error set.
  size_t write(const uint8_t *b, size_t size) override;

  // Writes a single byte. This returns 1 if there was no error, otherwise this
  // returns zero. If this returns zero then a write error will be set.
  //
  // This returns zero if there is already a write error set.
  size_t write(uint8_t b) override;

  // Writes a frame END marker. This behaves the same as the single-byte write.
  //
  // Note that this does not flush the stream.
  size_t writeEnd();

  // Flushes the stream. If there was a write error or if not all the bytes
  // could be sent then a write error will be set.
  //
  // This does nothing if there is already a write error set.
  void flush() override;

  // This will return a maximum of 1, currently, but this may change if read
  // buffering is ever implemented.
  int available() override;

  // Reads one character from the SLIP stream. All unknown escaped characters
  // are returned as-is, despite being a protocol violation.
  //
  // This will return -1 if there are no bytes available and -2 for
  // end-of-frame.
  int read() override;

  // Returns whether the last call to read() returned an END marker. This resets
  // to false the next time read() is called.
  bool isEnd() const {
    return isEND_;
  }

  // Peeks at one character from the SLIP stream. The character determination
  // logic is the same as for read(). This also returns -1 for no data and -2
  // for end-of-frame.
  int peek() override;

 private:
  // Writes out the complete buffer to the underlying stream. If there was a
  // short write then this shifts the buffer data. This returns whether the
  // write was successful.
  //
  // If ever a caller resets the write error, then the bytes remaining in the
  // buffer will still be correct. This is the reason for shifting any
  // unwritten bytes.
  bool writeBuf();

  // Encodes and writes a single byte to the buffer without first checking
  // whether there's been a write error. This returns whether the write was
  // successful, 1 for success and 0 for no success.
  size_t writeByte(uint8_t b);

  Stream &stream_;

  // Write buffer
  size_t bufSize_;
  uint8_t *buf_;
  size_t bufIndex_;

  // Character read state.
  bool inESC_;

  // Indicates whether the last read() call returned an END marker.
  bool isEND_;
};

#endif  // SLIPSTREAM_H_
