#ifndef __MOTION_DETECT_H__
#define __MOTION_DETECT_H__

#include "hi_type.h"
#include "hi_conf.h"

typedef void (*motion_alarm_cb_t)(HI_U32 u32Region, HI_BOOL bAlarm,
                                   void *pPriv);

HI_S32  motion_detect_init(const MOTION_CONF_S *pstConf);
HI_VOID motion_detect_deinit(void);
HI_S32  motion_detect_start(void);
HI_VOID motion_detect_stop(void);
HI_S32  motion_detect_set_region(HI_U32 u32Idx,
                                  const MOTION_REGION_CONF_S *pstRegion);
HI_S32  motion_detect_get_region(HI_U32 u32Idx,
                                  MOTION_REGION_CONF_S *pstRegion);
HI_S32  motion_detect_register_callback(motion_alarm_cb_t pfnCb, void *pPriv);
HI_BOOL motion_detect_is_alarming(void);

#endif /* __MOTION_DETECT_H__ */
