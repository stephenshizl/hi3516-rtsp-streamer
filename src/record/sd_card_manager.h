#ifndef __SD_CARD_MANAGER_H__
#define __SD_CARD_MANAGER_H__

#include "hi_type.h"

typedef struct {
    char    szDevice[64];
    char    szMountPoint[128];
    HI_U64  u64TotalSizeMb;
    HI_U64  u64FreeSizeMb;
    HI_BOOL bMounted;
    HI_BOOL bWritable;
    HI_BOOL bHealthy;
} SD_CARD_INFO_S;

HI_S32  sdcard_init(const char *pszMountPoint);
HI_VOID sdcard_deinit(void);
HI_S32  sdcard_mount(const char *pszDevice, const char *pszFsType);
HI_S32  sdcard_unmount(void);
HI_S32  sdcard_check(void);
HI_S32  sdcard_get_info(SD_CARD_INFO_S *pstInfo);
HI_BOOL sdcard_is_available(void);

#endif /* __SD_CARD_MANAGER_H__ */
