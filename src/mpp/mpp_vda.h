#ifndef __MPP_VDA_H__
#define __MPP_VDA_H__

#include "hi_type.h"
#include "hi_conf.h"

typedef void (*vda_result_cb_t)(HI_S32 s32Chn, HI_BOOL bAlarm, void *pPriv);

HI_S32  mpp_vda_init(const MOTION_CONF_S *pstConf);
HI_VOID mpp_vda_deinit(void);
HI_S32  mpp_vda_register_callback(vda_result_cb_t pfnCb, void *pPriv);

#endif /* __MPP_VDA_H__ */
