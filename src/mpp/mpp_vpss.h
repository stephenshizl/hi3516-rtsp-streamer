#ifndef __MPP_VPSS_H__
#define __MPP_VPSS_H__

#include "hi_type.h"
#include "hi_conf.h"

typedef struct {
    HI_U32  u32Width;
    HI_U32  u32Height;
    HI_U32  u32FrameRate;
    HI_BOOL bEnable;
} VPSS_CHN_CONF_S;

typedef struct {
    HI_U32         u32SrcWidth;
    HI_U32         u32SrcHeight;
    VPSS_CHN_CONF_S astChn[MAX_VPSS_CHN];
} VPSS_GRP_CONF_S;

HI_S32  mpp_vpss_init(const VIDEO_CONF_S *pstVideoConf);
HI_VOID mpp_vpss_deinit(void);

#endif /* __MPP_VPSS_H__ */
