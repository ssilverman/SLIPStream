// Library for reading and writing SLIP using an underlying Stream object.
// This implements [RFC 1055](https://tools.ietf.org/html/rfc1055).
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
#if __has_include(<cstdint>)
#include <cstdint>
#else
#include <stdint.h>
#endif

// Other includes
#include <Stream.h>

class SLIPStream : public Stream {
 public:
  // Read return values
  static constexpr int END_FRAME = -2;
  static constexpr int BAD_DATA  = -3;

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
  void flush() override;

  // This will return about half the underlying stream's available byte count,
  // under the conservative assumption that each character potentially occupies
  // two bytes in its encoded form. More bytes may actually be available.
  //
  // Note that a frame END marker is considered an available byte.
  int available() override;

  // Reads one character from the SLIP stream. All unknown escaped characters
  // are a protocol violation and considered corrupt data.
  //
  // This will return -1 if there are no bytes available, -2 for end-of-frame,
  // and -3 for corrupt data.
  //
  // The END condition can be tested with isEnd(). For corrupt data, tested with
  // isBadData(), the caller should read until the next END marker.
  int read() override;

  // Returns whether the last call to read() returned an END marker. This resets
  // to false the next time read() is called.
  bool isEnd() const {
    return isEND_;
  }

  // Returns whether the last call to read() encountered corrupt data. This
  // resets to false the next time read() is called.
  bool isBadData() const {
    return isBadData_;
  }

  // Peeks at one character from the SLIP stream. The character determination
  // logic is the same as for read(). This also returns -1 for no data, -2 for
  // end-of-frame, and -3 for corrupt data.
  int peek() override;

 private:
  // Encodes and writes an encoded byte to the underlying stream. This returns
  // whether the write was successful, 1 for success and 0 for no success.
  //
  // If the write was not successful then this stream's write error will be set.
  // This does not check if the underlying stream has a write error set, so the
  // caller will need to check. This avoids a check when doing multi-byte calls.
  size_t writeByte(uint8_t b);

  Stream &stream_;

  // Character read state.
  bool inESC_;

  // Indicates whether the last read() call returned an END marker.
  bool isEND_;

  // Indicates whether the last read() call encountered corrupt data.
  bool isBadData_;
};

#endif  // SLIPSTREAM_H_
