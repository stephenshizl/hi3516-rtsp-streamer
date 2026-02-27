#ifndef __RTSP_SERVER_H__
#define __RTSP_SERVER_H__

#include "hi_type.h"
#include "hi_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

HI_S32  rtsp_server_init(const NETWORK_CONF_S *pstNetConf);
HI_VOID rtsp_server_deinit(void);
HI_S32  rtsp_server_start(void);
HI_VOID rtsp_server_stop(void);

HI_S32  rtsp_server_add_stream(const char *pszName, HI_S32 s32VencChn,
                                VENC_TYPE_E enCodec, HI_U32 u32Fps);
HI_VOID rtsp_server_remove_stream(const char *pszName);

HI_S32  rtsp_server_feed_frame(HI_S32 s32VencChn, const HI_U8 *pData,
                                HI_U32 u32Len, HI_U64 u64Pts,
                                HI_BOOL bKeyFrame);

HI_S32  rtsp_server_get_client_count(const char *pszName);
HI_S32  rtsp_server_request_idr(HI_S32 s32VencChn);

#ifdef __cplusplus
}
#endif

#endif /* __RTSP_SERVER_H__ */
