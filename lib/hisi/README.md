# HiSilicon SDK Libraries

Place the Hi3516CV300 MPP shared/static libraries here.

Required libraries (from `Hi3516CV300_SDK_V2.0.4.0/mpp/out/lib/`):

- `libmpi.so` or `libmpi.a`
- `libVoiceEngine.so`
- `libresampler.so`
- `libaec.so`
- `libans.so`

Copy from the SDK:

```bash
cp $SDK_PATH/mpp/out/lib/libmpi.so      path/to/lib/hisi/
cp $SDK_PATH/mpp/out/lib/libVoiceEngine.so path/to/lib/hisi/
```

These are provided by HiSilicon under the SDK license.
