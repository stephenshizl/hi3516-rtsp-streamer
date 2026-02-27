# Live555 Libraries

Place the cross-compiled Live555 static libraries here:

- `libliveMedia.a`
- `libgroupsock.a`
- `libBasicUsageEnvironment.a`
- `libUsageEnvironment.a`

Build Live555 for ARM Hi3516CV300:

```bash
cd live555
./genMakefiles linux-hisiv300
make -j4
cp BasicUsageEnvironment/libBasicUsageEnvironment.a  path/to/lib/live555/
cp UsageEnvironment/libUsageEnvironment.a            path/to/lib/live555/
cp liveMedia/libliveMedia.a                          path/to/lib/live555/
cp groupsock/libgroupsock.a                          path/to/lib/live555/
```
