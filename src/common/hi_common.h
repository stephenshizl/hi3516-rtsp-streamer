#ifndef __HI_COMMON_H__
#define __HI_COMMON_H__

#include "hi_type.h"

#define MAX_MOTION_REGIONS  4
#define MAX_VENC_CHN        4
#define MAX_VPSS_GRP        4
#define MAX_VPSS_CHN        4

#define VENC_CHN_MAIN       0
#define VENC_CHN_SUB        1
#define VPSS_GRP_MAIN       0
#define VPSS_CHN_MAIN       0
#define VPSS_CHN_SUB        1
#define VPSS_CHN_VDA        2
#define VI_PIPE_MAIN        0
#define VI_CHN_MAIN         0
#define VDA_CHN_MAIN        0

#define DEFAULT_CONF_FILE   "/etc/ipcamera.conf"
#define DEFAULT_RECORD_PATH "/mnt/sdcard/record"
#define DEFAULT_RTSP_PORT   554

#define ARRAY_SIZE(x)       (sizeof(x) / sizeof((x)[0]))
#define ALIGN_UP(x, a)      (((x) + ((a) - 1)) & ~((a) - 1))
#define ALIGN_DOWN(x, a)    ((x) & ~((a) - 1))

#define CHECK_RET(ret)                          \
    do {                                        \
        if (HI_SUCCESS != (ret)) {              \
            HI_LOG_E("ret=0x%x", (ret));        \
            goto EXIT;                          \
        }                                       \
    } while (0)

#define CHECK_RET_RETURN(ret)                   \
    do {                                        \
        if (HI_SUCCESS != (ret)) {              \
            HI_LOG_E("ret=0x%x", (ret));        \
            return (ret);                       \
        }                                       \
    } while (0)

#define CHECK_NULL_RETURN(ptr)                  \
    do {                                        \
        if (HI_NULL == (ptr)) {                 \
            HI_LOG_E("null pointer");           \
            return HI_FAILURE;                  \
        }                                       \
    } while (0)

typedef enum {
    VENC_TYPE_H264 = 0,
    VENC_TYPE_H265 = 1,
    VENC_TYPE_MJPEG = 2,
    VENC_TYPE_BUTT,
} VENC_TYPE_E;

typedef enum {
    RC_MODE_CBR = 0,
    RC_MODE_VBR = 1,
    RC_MODE_AVBR = 2,
    RC_MODE_BUTT,
} RC_MODE_E;

typedef enum {
    PIXEL_FORMAT_YVU_SEMIPLANAR_420 = 0,
    PIXEL_FORMAT_YVU_SEMIPLANAR_422 = 1,
    PIXEL_FORMAT_BUTT,
} PIXEL_FORMAT_E;

#endif /* __HI_COMMON_H__ */
