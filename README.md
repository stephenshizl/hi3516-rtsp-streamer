# Hi3516CV300 IPCamera RTSP Streamer

A complete IPCamera RTSP streaming solution for the HiSilicon Hi3516CV300 platform, featuring dual-stream H.264/H.265 encoding, Live555-based RTSP server, motion detection via VDA, and SD card recording.

## Features

- **Dual-stream encoding**: Main stream (1080P) + Sub stream (720P)
- **H.264 / H.265 support**: Configurable via `ipcamera.conf`
- **Live555 RTSP server**: Standard-compliant streaming on port 554
- **Motion detection**: Hardware VDA with configurable regions and sensitivity
- **SD card recording**: Continuous, motion-triggered, or manual recording
- **Circular storage**: Automatic cleanup of old recordings to maintain free space
- **INI configuration**: Runtime-configurable via `/etc/ipcamera.conf`
- **Signal-safe shutdown**: Graceful cleanup on SIGINT/SIGTERM

## RTSP URLs

| Stream | URL | Resolution | Default Bitrate |
|--------|-----|-----------|-----------------|
| Main   | `rtsp://<ip>:554/stream0` | 1920×1080 | 4096 Kbps |
| Sub    | `rtsp://<ip>:554/stream1` | 1280×720  | 1024 Kbps |

### Test playback
```bash
ffplay rtsp://192.168.1.100/stream0
vlc rtsp://192.168.1.100/stream1
```

## Build

See [docs/BUILD.md](docs/BUILD.md) for detailed build instructions.

```bash
# Quick build (requires cross-compiler)
export CROSS_COMPILE=arm-hisiv300-linux-
export SDK_PATH=/opt/Hi3516CV300_SDK_V2.0.4.0
make -j4
```

## Deployment

```bash
# 1. Initialize hardware
./scripts/system_init.sh

# 2. Start streamer
./scripts/start.sh

# 3. Stop streamer
./scripts/stop.sh
```

## Configuration

Edit `/etc/ipcamera.conf` to change stream parameters, network settings, motion detection regions, and recording options. See [docs/CONFIG.md](docs/CONFIG.md) for all options.

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                       Main Process                       │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌────────┐  │
│  │  Config  │  │  Signal  │  │ SD Card  │  │ Circ.  │  │
│  │  Parser  │  │ Handler  │  │ Monitor  │  │Storage │  │
│  └──────────┘  └──────────┘  └──────────┘  └────────┘  │
└─────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────┐
│                     MPP Pipeline                         │
│  Sensor → VI → VPSS → VENC(Main/Sub)                    │
│                  └──→ VDA (Motion Detect)                │
└─────────────────────────────────────────────────────────┘
         │                      │
         ▼                      ▼
┌────────────────┐    ┌──────────────────┐
│  Live555 RTSP  │    │  Record Manager  │
│  Server Thread │    │  (MP4 to SD)     │
└────────────────┘    └──────────────────┘
```

## Directory Structure

```
hi3516-rtsp-streamer/
├── Makefile               # Top-level build
├── build/                 # Compiler toolchain configs
├── src/
│   ├── main.c             # Entry point
│   ├── common/            # Types, logging, utilities
│   ├── mpp/               # VI, VPSS, VENC, VDA, VB, SYS
│   ├── rtsp/              # Live555 RTSP server
│   ├── record/            # Recording & storage
│   ├── motion/            # Motion detection
│   └── config/            # Config parser + default conf
├── include/               # External headers (live555, hisi)
├── lib/                   # External libs (live555, hisi)
├── scripts/               # init/start/stop scripts
└── docs/                  # Documentation
```

## License

See LICENSE file.

## Android Device Admin App

An Android 13 device administrator application is also provided as a companion to the Hi3516 RTSP streamer. See [android-device-admin/README.md](android-device-admin/README.md) for details.

This Android app provides:
- Device security management for Hi3516-based Android devices
- Remote device locking and data wipe capabilities
- Password policy enforcement
- Camera control and monitoring
- Security event notifications

The Android app is independent of the C/C++ RTSP streamer and can be used separately on Android 13+ devices.
