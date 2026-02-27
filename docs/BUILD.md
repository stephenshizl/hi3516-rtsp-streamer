# Build Instructions

## Prerequisites

### Cross-Compiler

Install the HiSilicon cross-compiler toolchain:

```bash
tar -xf arm-hisiv300-linux.tgz -C /opt/
source /opt/arm-hisiv300-linux/environment-setup
```

Verify:
```bash
arm-hisiv300-linux-gcc --version
```

### Hi3516CV300 SDK

Extract the SDK to `/opt/Hi3516CV300_SDK_V2.0.4.0`:

```bash
tar -xf Hi3516CV300_SDK_V2.0.4.0.tgz -C /opt/
```

### Live555

Build Live555 for ARM:

```bash
cd live555
./genMakefiles linux-hisiv300
make -j4

# Copy libraries
cp BasicUsageEnvironment/libBasicUsageEnvironment.a  ../lib/live555/
cp UsageEnvironment/libUsageEnvironment.a            ../lib/live555/
cp liveMedia/libliveMedia.a                          ../lib/live555/
cp groupsock/libgroupsock.a                          ../lib/live555/

# Copy headers
cp -r liveMedia/include/*            ../include/live555/
cp -r UsageEnvironment/include/*     ../include/live555/
cp -r BasicUsageEnvironment/include/* ../include/live555/
cp -r groupsock/include/*            ../include/live555/
```

## Build

```bash
cd hi3516-rtsp-streamer

# Set SDK path (if not default)
export SDK_PATH=/opt/Hi3516CV300_SDK_V2.0.4.0

# Build
make clean
make -j4

# Output
ls -la output/ipcamera
```

## Stub/Host Build (without SDK)

For testing on host without the actual SDK, the code compiles cleanly
because all HiSilicon SDK calls are guarded by `#ifdef HISI_SDK_AVAILABLE`.
Without defining that macro, all SDK calls are no-ops and the program
runs in stub mode.

```bash
# Build with native GCC for testing
make CC=gcc CXX=g++ CROSS_COMPILE="" -j4
```

## Makefile Variables

| Variable        | Default                             | Description               |
|-----------------|-------------------------------------|---------------------------|
| `CROSS_COMPILE` | `arm-hisiv300-linux-`               | Toolchain prefix          |
| `SDK_PATH`      | `/opt/Hi3516CV300_SDK_V2.0.4.0`     | SDK root directory        |
| `SENSOR_TYPE`   | `IMX335_MIPI_5M_30FPS`              | Sensor type               |

## Deploy to Target Board

```bash
# Strip binary
arm-hisiv300-linux-strip output/ipcamera

# Upload
scp output/ipcamera       root@192.168.1.100:/usr/bin/
scp src/config/ipcamera.conf root@192.168.1.100:/etc/

# Upload shared libraries if dynamic linking used
scp lib/live555/*.so      root@192.168.1.100:/lib/
```
