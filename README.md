xcb-imdkit
==============================
xcb-imdkit is an implementation of xim protocol in xcb, comparing with the
implementation of IMDkit with Xlib, and xim inside Xlib, it has less memory
foot print, better performance, and safer on malformed client.

And not to say it's all asynchronous and it works with xcb.

Though, it doesn't have complete protocol support, since some part of the protocol
is never used. Those parts include:
- XIM_AUTH (IMDkit doesn't implement this, Xlib only has stub, so still no implementation.)
- XIM_STR_CONVERSION (Synchronous protocol, but not used anywhere)
- Only X transport is supported.
