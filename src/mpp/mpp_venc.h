#ifndef __MPP_VENC_H__
#define __MPP_VENC_H__

#include "hi_type.h"
#include "hi_conf.h"

#define VENC_STREAM_BUF_SIZE    (2 * 1024 * 1024)
#define VENC_MAX_NALU_COUNT     64

typedef struct {
    HI_U8   *pData;
    HI_U32   u32Len;
    HI_U64   u64Pts;
    HI_BOOL  bKeyFrame;
    HI_U32   u32NaluType;
} VENC_FRAME_S;

typedef void (*venc_frame_cb_t)(HI_S32 s32Chn, const VENC_FRAME_S *pstFrame,
                                 void *pPrivData);

HI_S32  mpp_venc_init(const STREAM_CONF_S *pstMain, const STREAM_CONF_S *pstSub);
HI_VOID mpp_venc_deinit(void);

HI_S32  mpp_venc_register_callback(HI_S32 s32Chn, venc_frame_cb_t pfnCb,
                                    void *pPrivData);
HI_S32  mpp_venc_unregister_callback(HI_S32 s32Chn);
HI_S32  mpp_venc_request_idr(HI_S32 s32Chn);
HI_S32  mpp_venc_set_bitrate(HI_S32 s32Chn, HI_U32 u32BitRate);

#endif /* __MPP_VENC_H__ */
