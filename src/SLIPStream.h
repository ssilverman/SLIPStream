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
  // Creates a new SLIPStream object.
  SLIPStream(Stream &stream);

  virtual ~SLIPStream() = default;

  // Returns the number of bytes that can be written without blocking.
  // Internally, this assumes that each byte will be written as two, so this
  // returns half the underlying stream's non-blocking write size.
  int availableForWrite() override;

  // Writes a range of bytes. This may not write all the bytes if there was a
  // write error or if the underlying stream was unable to write all the bytes.
  // If this is the case then a write error will be set. This will return,
  // however, the actual number of bytes written.
  size_t write(const uint8_t *b, size_t size) override;

  // Writes a single byte. This returns 1 if there was no error, otherwise this
  // returns zero. If this returns zero then a write error will be set.
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
  // Encodes and writes an encoded byte to the underlying stream. This returns
  // whether the write was successful, 1 for success and 0 for no success.
  //
  // If the write was not successful or if the underlying stream has a write
  // error then this stream's write error will be set.
  size_t writeByte(uint8_t b);

  Stream &stream_;

  // Character read state.
  bool inESC_;

  // Indicates whether the last read() call returned an END marker.
  bool isEND_;
};

#endif  // SLIPSTREAM_H_
