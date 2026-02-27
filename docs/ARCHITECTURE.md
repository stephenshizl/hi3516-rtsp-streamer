# Architecture Design

## Overview

The IPCamera streamer is a single-process, multi-threaded application for the Hi3516CV300 SoC. It uses the HiSilicon MPP framework for hardware video capture/encoding and Live555 for RTSP streaming.

## MPP Pipeline

```
 Sensor (MIPI/LVDS)
      │
      ▼
 ┌─────────┐
 │   VI    │  Video Input - receives raw frames from sensor
 │ Dev 0   │  Pipe 0, Chn 0
 └────┬────┘
      │ (bind)
      ▼
 ┌─────────┐
 │  VPSS   │  Video Processing Sub-System
 │ Group 0 │  - NR (noise reduction)
 │  Chn 0  │──── (1920×1080) ──→ VENC Chn 0 (Main Stream)
 │  Chn 1  │──── (1280×720)  ──→ VENC Chn 1 (Sub Stream)
 │  Chn 2  │──── (352×288)   ──→ VDA Chn 0 (Motion Detect)
 └─────────┘
```

## Thread Model

| Thread | Function |
|--------|----------|
| Main thread | Init/deinit, signal handling, periodic maintenance |
| VENC stream thread (x2) | Poll VENC FDs, dispatch frames to RTSP and record |
| Live555 event loop | Handle RTSP sessions, RTP packetization, client I/O |
| VDA poll thread | Read motion detection results, trigger callbacks |

## Data Flow

```
VENC Chn 0 ──→ on_venc_frame() ──┬──→ rtsp_server_feed_frame() ──→ H264StreamSource
                                  └──→ record_manager_feed_frame() ──→ MP4 file
VENC Chn 1 ──→ on_venc_frame() ──┬──→ rtsp_server_feed_frame() ──→ H264StreamSource
                                  └──→ (ignored by recorder, main only)
VDA Chn 0  ──→ on_vda_result()  ──→ motion_detect (debounce) ──→ on_motion_alarm()
                                                                    └──→ record_manager_on_motion()
```

## Memory Layout

| Region     | Size     | Usage              |
|------------|----------|--------------------|
| MMZ        | 256 MB   | MPP frame buffers  |
| Main VB    | ~30 MB   | 10 × 1080P frames  |
| Sub VB     | ~13 MB   | 10 × 720P frames   |
| VENC VB    | ~4 MB    | Encoded stream buf |
| VDA VB     | ~2 MB    | VDA analysis buf   |
| RTSP queue | ~64 MB   | Per-client RTP buf |

## Recording State Machine

```
         IDLE
          │ motion alarm or continuous mode
          ▼
      WAITING (buffering pre-record frames)
          │ keyframe received
          ▼
      WRITING ────────────── slice time expired ──→ close & open new file
          │
          │ mode=motion && post_record timeout
          ▼
         IDLE
```

## Error Handling

- All MPP API calls check return values
- Failed SDK calls log error with hexadecimal error code
- Non-fatal failures (VDA, SD card) log warning and continue
- Fatal failures in MPP pipeline result in orderly shutdown
- SIGINT/SIGTERM trigger graceful shutdown sequence
