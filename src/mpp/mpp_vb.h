#ifndef __MPP_VB_H__
#define __MPP_VB_H__

#include "hi_type.h"
#include "hi_conf.h"

#define VB_POOL_MAIN_CHN    0
#define VB_POOL_SUB_CHN     1
#define VB_POOL_VENC        2
#define VB_POOL_VDA         3
#define VB_POOL_MAX_CNT     4

#define VB_BLK_MAIN_CNT     10
#define VB_BLK_SUB_CNT      10
#define VB_BLK_VENC_CNT     8
#define VB_BLK_VDA_CNT      4

HI_S32  mpp_vb_init(const IPCAMERA_CONF_S *pstConf);
HI_VOID mpp_vb_deinit(void);

HI_U32  mpp_vb_calc_blk_size(HI_U32 u32Width, HI_U32 u32Height,
                               PIXEL_FORMAT_E enPixFmt, HI_U32 u32Align);

#endif /* __MPP_VB_H__ */
