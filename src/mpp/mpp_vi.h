#ifndef __MPP_VI_H__
#define __MPP_VI_H__

#include "hi_type.h"
#include "hi_conf.h"

typedef enum {
    VI_MODE_MIPI  = 0,
    VI_MODE_LVDS  = 1,
    VI_MODE_BT656 = 2,
    VI_MODE_BUTT,
} VI_MODE_E;

typedef enum {
    SENSOR_IMX335 = 0,
    SENSOR_IMX307 = 1,
    SENSOR_OS05A  = 2,
    SENSOR_BUTT,
} SENSOR_TYPE_E;

typedef struct {
    HI_U32       u32Width;
    HI_U32       u32Height;
    HI_U32       u32FrameRate;
    VI_MODE_E    enViMode;
    SENSOR_TYPE_E enSensor;
    PIXEL_FORMAT_E enPixFmt;
    HI_BOOL      bWdrMode;
} VI_CONF_S;

HI_S32  mpp_vi_init(const VI_CONF_S *pstConf);
HI_VOID mpp_vi_deinit(void);

#endif /* __MPP_VI_H__ */
