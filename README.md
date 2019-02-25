# Readme for SLIPStream v1.0.1

This is a library for sending and receiving SLIP using any object that
implements `Stream`. It implements
[RFC 1055](https://tools.ietf.org/html/rfc1055).

Framing is important because it prevents streamed data from becoming
unsynchronized. This library provides a well-known way to frame data over
any stream.

Notable features:

* Can seamlessly be used as a `Stream` or `Print` object.
* Read and write availablility functions work.
* Properly sets the write error state, inclusive of tracking the underlying
  stream's write error state.
* Provides a way to detect protocol violations.
* There are nuances to the SLIP protocol and this library aims to get
  them right.

## How to use

To use this library, simply treat it as you would any `Stream` object. When
constructing an instance of `SLIPStream`, you need to pass in a reference to
an existing `Stream` object, for example `Serial1`.

Additional functions:

1. `writeEnd()`:  This writes an END marker for the current frame.
2. `isEnd()`: Determines if the last `read()` call detected a frame END marker.
3. `isBadData()`: Determines if the last `read()` call returned data that was
   involved in a protocol violation. That is, an escaped character that
   shouldn't have been escaped.
4. `read(uint8_t *, size_t)`: A multi-byte read implementation, intended to
   replace `readBytes` if the caller desires to know if corrupt data has
   been read.

### Writing data

Simply send data as normal by using one of the `write` calls. To send an END
marker for the current frame, call `writeEnd()`. Note that the stream is not
flushed, so to ensure all the bytes are sent, including the END marker, call
`flush()`.

To determine the number of characters that can be written without blocking, call
the `availableForWrite()` function. This makes the conservative assumption that
each character will be written as two bytes, even though the majority of
characters are sent as one byte. Therefore, treat this number as a minimum. It
just returns the underlying stream's availability divided by two.

#### Write errors

This class tracks the write error state of the underlying stream, in addition
to tracking write counts. The write error state will be set if:

1. Not all bytes are written successfully. This may happen when single-byte
   writes return zero or when multi-byte writes return a number less than the
   specified write count.
2. The underlying stream has its write error set after any write or flush call.

Note that clearing a write error by setting it to zero is possible, but be aware
that only the first byte of a two-byte character may have been sent (the ESC
byte). This error condition _may_ be cleared by sending two frame END markers,
depending on how the SLIP receiver behaves. The first may or may not be
interpreted as a regular byte, depending on how the receiver processes protocol
violations (if there was indeed a dangling ESC byte sent), but the second should
be interpreted as a proper frame END marker because it's not preceded by an
ESC byte.

### Reading data

Data can be received normally using the `read()` call. This will, as usual,
return -1 when no data is available, but will return `SLIPStream::END_FRAME`
when a frame END marker has been received. The value of this constant is -2.

In other words, stopping conditions are indicated by negative values, and so
should work with code that only checks for negative values instead of just -1.

When an END marker is received, the "END" condition is set. If corrupt SLIP data
was received, that is, unnecessarily escaped bytes are in the data, then they
are returned as normal and the "Bad Data" condition is set. More details on
these two conditions are below.

#### Read conditions

There are two additional conditions after a `read()` call:
1. A frame END marker was encountered.
2. Corrupt data was received.

The END marker can be detected with the `isEnd()` call and corrupt data can be
detected with the `isBadData()` call. Both functions only apply to the latest
call to `read()`. In other words, if `read()` is called again, then both
condition-testing functions may return new results.

If `isEnd()` returns `true` then the last `read()` call returned -2. If
`isBadData()` returns `true` then the last `read()` call returned a valid byte
value, but it was inappropriately escaped.

A note on data corruption: There are some cases where the data is considered
corrupt. This happens when unnecessarily escaped bytes are received. That is,
bytes that are preceded by a SLIP escape byte that don't need to be. The only
two valid escapable bytes in the SLIP protocol are ESC and END. If an improperly
escaped byte is received then it is returned as normal, however the
`isBadData()` function will return `true`.

#### Reading multiple bytes

It is suggested that `readBytes` in the `Stream` class not be used to read
multiple bytes for when there's any likelihood of protocol violations. This
is because if corrupt data was received somewhere in the middle of the bytes
and not the last byte, then it's not possible to detect that data corruption
has occurred, or which bytes are considered corrupt.

The `isBadData()` call only applies to the latest `read()` call, and `readBytes`
is not overridable, so it's not possible to intercept its behaviour.

To fix this, there is a new multi-byte `read` function that will stop under the
usual conditions, no more bytes and an END marker, but will also stop when
corrupt data is received. This way, the `isEnd()` and `isBadData()` calls will
still behave as expected. Note that this function is defined in the `SLIPStream`
class and not in the `Stream` class.

Having said all that, `readBytes` will still work, but it won't be possible to
determine which data is corrupt, but this may not be an issue.

#### Read errors

The `isBadData()` call's return value only applies to the most recent call to
`read()`.

It was decided to return a non-negative value for corrupt data because the
caller can still decide what to do with corrupt data but not have to use the
stopping logic for what is essentially a different concern. In addition, the
`readBytes()` call will still properly stop only on frame END markers and EOF
conditions.

## Technical notes

### What this doesn't do

This library does not provide:

* The ability to determine frame length.
* Checksumming.
* Buffering.

### Write errors

If any bytes cannot be written or if the underlying `Stream` object has its
write error set, then the `SLIPStream` object's write error will be set.

### Reading corrupt data

It was decided to have corrupt data not cause `read()` to return a negative
value for these reasons:

1. In some cases it might be useful to examine what data was being received.
   Dropping it on the floor loses this information.
2. Implementation experience with code that uses this class showed that it was
   slightly simpler to process the logic this way. For example, when skipping
   bytes, a common paradigm is:
   ```c++
   while (stream.read() >= 0) {
     // Skip while available
   }
   ```
3. For properly-written SLIP encoders, seeing this error in a connected receiver
   is unlikely, and adding an extra error condition for this case just doesn't
   seem necessary.

### Two cautions

Because the SLIP protocol encodes some characters as two bytes, caution must be
used when deciding availability with `peek()` and determining correctness with
the multi-byte `read`.

Normally, when `peek()` returns -1, it means no bytes are available in the
stream. However, an incomplete 2-byte character---that is, only the first byte
is available---means that the whole character is _effectively_ not available.
Therefore, code that determines data availability by relying on just `peek()`
returning -1 without calling `read` will receive -1 from `peek()` forever, and
the stream will never advance.

The second caution is for using the return value from the multi-byte `read` to
determine whether corrupt data was received. The read stops when a corrupt byte
has been received, including the last requested byte. Therefore, even though it
may appear that all requested bytes have been received, the last byte may still
be invalid. This condition can be checked by calling `isBadData()`.

## Code style

Code style for this project mostly follows the
[Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).

## References

1. [RFC 1055](https://tools.ietf.org/html/rfc1055).

---

Copyright (c) 2018-2019 Shawn Silverman
