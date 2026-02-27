# Live555 Headers

Place the compiled Live555 headers here.

Required headers from the Live555 source tree:

- `liveMedia/include/*.hh`
- `UsageEnvironment/include/*.hh`
- `BasicUsageEnvironment/include/*.hh`
- `groupsock/include/*.hh`

After building Live555 for the target architecture, copy the headers here
and the static libraries to `lib/live555/`.

Add `-DLIVE555_AVAILABLE` to `CXXFLAGS` in `build/sdk_path.mk` after placing
the headers and libraries.
