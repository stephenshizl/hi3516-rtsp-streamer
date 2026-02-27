#ifndef __RECORD_MANAGER_H__
#define __RECORD_MANAGER_H__

#include "hi_type.h"
#include "hi_conf.h"

HI_S32  record_manager_init(const RECORD_CONF_S *pstConf);
HI_VOID record_manager_deinit(void);
HI_S32  record_manager_start(void);
HI_VOID record_manager_stop(void);
HI_S32  record_manager_feed_frame(HI_S32 s32Chn, const HI_U8 *pData,
                                   HI_U32 u32Len, HI_U64 u64Pts,
                                   HI_BOOL bKeyFrame);
HI_VOID record_manager_on_motion(HI_BOOL bAlarm);
HI_BOOL record_manager_is_recording(void);

#endif /* __RECORD_MANAGER_H__ */
