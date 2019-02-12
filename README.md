# Readme for SLIPStream v1.0.0

This is a library for sending and receiving SLIP using any object that
implements `Stream`. It implements
[RFC 1055](https://tools.ietf.org/html/rfc1055).

Framing is important because it prevents streamed data from becoming
unsynchronized. This library provides a well-known way to frame data over
any stream.

## How to use

To use this library, simply treat it as you would any `Stream` object. When
constructing an instance of `SLIPStream`, you need to pass in a reference to
an existing `Stream` object, for example `Serial1` or an instance of `Udp`.

The only additional function is `writeEnd`. This writes an END marker for the
current frame.

## Technical notes

If any bytes cannot be written or if the underlying `Stream` object has its
write error set, then the `SLIPStream` object's write error will be set. In
fact, if the write error is set, then all write functions (including flush)
will do nothing.


---

Copyright (c) 2018-2019 Shawn Silverman
