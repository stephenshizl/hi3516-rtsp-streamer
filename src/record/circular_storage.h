#ifndef __CIRCULAR_STORAGE_H__
#define __CIRCULAR_STORAGE_H__

#include "hi_type.h"

typedef struct {
    char    szPath[128];
    HI_U64  u64MinFreeSpaceMb;
    HI_U32  u32MaxFileAgeDays;
} CIRCULAR_STORAGE_CONF_S;

HI_S32  circular_storage_init(const CIRCULAR_STORAGE_CONF_S *pstConf);
HI_VOID circular_storage_deinit(void);
HI_S32  circular_storage_check(void);
HI_S32  circular_storage_add_file(const char *pszFilePath);
HI_S32  circular_storage_remove_file(const char *pszFilePath);
HI_S64  circular_storage_get_free_mb(void);

#endif /* __CIRCULAR_STORAGE_H__ */
