#ifndef __HI_CONF_H__
#define __HI_CONF_H__

#include "hi_type.h"
#include "hi_common.h"

typedef struct {
    HI_U32       u32Width;
    HI_U32       u32Height;
    HI_U32       u32BitRate;
    HI_U32       u32FrameRate;
    HI_U32       u32Gop;
    VENC_TYPE_E  enCodec;
    RC_MODE_E    enRcMode;
    HI_U32       u32Profile;
} STREAM_CONF_S;

typedef struct {
    STREAM_CONF_S stMain;
    STREAM_CONF_S stSub;
    HI_U32        u32SensorFps;
} VIDEO_CONF_S;

typedef struct {
    char     szIp[16];
    char     szNetmask[16];
    char     szGateway[16];
    char     szRtspUser[64];
    char     szRtspPass[64];
    HI_U16   u16RtspPort;
    HI_BOOL  bRtspAuth;
} NETWORK_CONF_S;

typedef struct {
    HI_S32  s32X;
    HI_S32  s32Y;
    HI_U32  u32Width;
    HI_U32  u32Height;
    HI_BOOL bEnable;
    HI_U32  u32Sensitivity;
    HI_U32  u32Threshold;
} MOTION_REGION_CONF_S;

typedef struct {
    HI_BOOL              bEnable;
    HI_U32               u32RegionCnt;
    MOTION_REGION_CONF_S astRegions[MAX_MOTION_REGIONS];
} MOTION_CONF_S;

typedef enum {
    RECORD_MODE_MANUAL     = 0,
    RECORD_MODE_MOTION     = 1,
    RECORD_MODE_SCHEDULE   = 2,
    RECORD_MODE_CONTINUOUS = 3,
    RECORD_MODE_BUTT,
} RECORD_MODE_E;

typedef struct {
    RECORD_MODE_E enMode;
    HI_U32        u32SliceTime;
    HI_U32        u32MaxDays;
    HI_U32        u32PreRecord;
    HI_U32        u32PostRecord;
    HI_U64        u64MinFreeSpaceMb;
    char          szMountPoint[128];
    HI_BOOL       bEnable;
} RECORD_CONF_S;

typedef struct {
    VIDEO_CONF_S   video;
    NETWORK_CONF_S network;
    MOTION_CONF_S  motion;
    RECORD_CONF_S  record;
} IPCAMERA_CONF_S;

void ipcamera_conf_set_default(IPCAMERA_CONF_S *pstConf);

#endif /* __HI_CONF_H__ */
