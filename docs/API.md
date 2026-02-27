# API Reference

## MPP System (`mpp_sys`)

```c
HI_S32  mpp_sys_init(void);
HI_VOID mpp_sys_deinit(void);
```

## Video Buffer Pool (`mpp_vb`)

```c
HI_S32  mpp_vb_init(const IPCAMERA_CONF_S *pstConf);
HI_VOID mpp_vb_deinit(void);
HI_U32  mpp_vb_calc_blk_size(HI_U32 width, HI_U32 height,
                              PIXEL_FORMAT_E fmt, HI_U32 align);
```

## Video Input (`mpp_vi`)

```c
HI_S32  mpp_vi_init(const VI_CONF_S *pstConf);
HI_VOID mpp_vi_deinit(void);
```

## Video Processing (`mpp_vpss`)

```c
HI_S32  mpp_vpss_init(const VIDEO_CONF_S *pstVideoConf);
HI_VOID mpp_vpss_deinit(void);
```

## Video Encoder (`mpp_venc`)

```c
HI_S32  mpp_venc_init(const STREAM_CONF_S *pstMain, const STREAM_CONF_S *pstSub);
HI_VOID mpp_venc_deinit(void);

/* Register callback to receive encoded frames */
HI_S32  mpp_venc_register_callback(HI_S32 chn, venc_frame_cb_t cb, void *priv);
HI_S32  mpp_venc_unregister_callback(HI_S32 chn);

/* Force IDR frame generation */
HI_S32  mpp_venc_request_idr(HI_S32 chn);

/* Dynamic bitrate change */
HI_S32  mpp_venc_set_bitrate(HI_S32 chn, HI_U32 kbps);
```

Callback signature:
```c
typedef void (*venc_frame_cb_t)(HI_S32 chn, const VENC_FRAME_S *frame, void *priv);
```

## Motion Detection (`motion_detect`)

```c
HI_S32  motion_detect_init(const MOTION_CONF_S *pstConf);
HI_VOID motion_detect_deinit(void);
HI_S32  motion_detect_start(void);
HI_VOID motion_detect_stop(void);

/* Configure detection regions at runtime */
HI_S32  motion_detect_set_region(HI_U32 idx, const MOTION_REGION_CONF_S *region);
HI_S32  motion_detect_get_region(HI_U32 idx, MOTION_REGION_CONF_S *region);

/* Register alarm callback */
HI_S32  motion_detect_register_callback(motion_alarm_cb_t cb, void *priv);

/* Query current state */
HI_BOOL motion_detect_is_alarming(void);
```

Callback signature:
```c
typedef void (*motion_alarm_cb_t)(HI_U32 region, HI_BOOL alarm, void *priv);
```

## RTSP Server (`rtsp_server`)

```c
HI_S32  rtsp_server_init(const NETWORK_CONF_S *pstNetConf);
HI_VOID rtsp_server_deinit(void);
HI_S32  rtsp_server_start(void);
HI_VOID rtsp_server_stop(void);

/* Register a stream (must be called before start) */
HI_S32  rtsp_server_add_stream(const char *name, HI_S32 vencChn,
                               VENC_TYPE_E codec, HI_U32 fps);
HI_VOID rtsp_server_remove_stream(const char *name);

/* Push encoded frame data */
HI_S32  rtsp_server_feed_frame(HI_S32 vencChn, const HI_U8 *data,
                               HI_U32 len, HI_U64 pts, HI_BOOL keyFrame);
```

## Record Manager (`record_manager`)

```c
HI_S32  record_manager_init(const RECORD_CONF_S *pstConf);
HI_VOID record_manager_deinit(void);
HI_S32  record_manager_start(void);
HI_VOID record_manager_stop(void);

/* Feed encoded frames from VENC callback */
HI_S32  record_manager_feed_frame(HI_S32 chn, const HI_U8 *data,
                                  HI_U32 len, HI_U64 pts, HI_BOOL keyFrame);

/* Trigger motion-based recording */
HI_VOID record_manager_on_motion(HI_BOOL alarm);

/* Query recording state */
HI_BOOL record_manager_is_recording(void);
```

## Configuration (`config_parser`)

```c
HI_S32  config_parse(const char *file, IPCAMERA_CONF_S *conf);
HI_S32  config_save(const char *file, const IPCAMERA_CONF_S *conf);
HI_VOID ipcamera_conf_set_default(IPCAMERA_CONF_S *conf);
```
