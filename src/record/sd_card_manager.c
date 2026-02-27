#include "sd_card_manager.h"
#include "hi_log.h"
#include "hi_utils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/mount.h>
#include <errno.h>
#include <mntent.h>

#define SD_CARD_CHECK_PERIOD_S  10

static SD_CARD_INFO_S g_stSdInfo;
static HI_BOOL        g_bInitialized = HI_FALSE;

static HI_BOOL check_mounted(const char *pszMountPoint)
{
    FILE         *fp;
    struct mntent *ent;
    HI_BOOL       bMounted = HI_FALSE;

    fp = setmntent("/proc/mounts", "r");
    if (HI_NULL == fp) {
        return HI_FALSE;
    }

    while ((ent = getmntent(fp)) != HI_NULL) {
        if (strcmp(ent->mnt_dir, pszMountPoint) == 0) {
            bMounted = HI_TRUE;
            strncpy(g_stSdInfo.szDevice, ent->mnt_fsname,
                    sizeof(g_stSdInfo.szDevice) - 1);
            break;
        }
    }

    endmntent(fp);
    return bMounted;
}

static HI_S32 update_space_info(void)
{
    struct statvfs svfs;

    if (!g_stSdInfo.bMounted) {
        return HI_FAILURE;
    }

    if (statvfs(g_stSdInfo.szMountPoint, &svfs) != 0) {
        HI_LOG_W("statvfs %s failed: %s", g_stSdInfo.szMountPoint,
                 strerror(errno));
        return HI_FAILURE;
    }

    g_stSdInfo.u64TotalSizeMb = (HI_U64)svfs.f_blocks * svfs.f_frsize / (1024 * 1024);
    g_stSdInfo.u64FreeSizeMb  = (HI_U64)svfs.f_bavail * svfs.f_frsize / (1024 * 1024);

    return HI_SUCCESS;
}

HI_S32 sdcard_init(const char *pszMountPoint)
{
    if (HI_NULL == pszMountPoint) {
        return HI_FAILURE;
    }

    memset(&g_stSdInfo, 0, sizeof(g_stSdInfo));
    strncpy(g_stSdInfo.szMountPoint, pszMountPoint,
            sizeof(g_stSdInfo.szMountPoint) - 1);

    if (hi_mkdir_p(pszMountPoint, 0755) != HI_SUCCESS) {
        HI_LOG_W("mkdir %s failed", pszMountPoint);
    }

    g_stSdInfo.bMounted = check_mounted(pszMountPoint);
    if (g_stSdInfo.bMounted) {
        update_space_info();
        g_stSdInfo.bWritable = HI_TRUE;
        g_stSdInfo.bHealthy  = HI_TRUE;
        HI_LOG_I("SD card already mounted: total=%lluMB free=%lluMB",
                 (unsigned long long)g_stSdInfo.u64TotalSizeMb,
                 (unsigned long long)g_stSdInfo.u64FreeSizeMb);
    } else {
        HI_LOG_I("SD card not mounted at %s", pszMountPoint);
    }

    g_bInitialized = HI_TRUE;
    return HI_SUCCESS;
}

HI_VOID sdcard_deinit(void)
{
    g_bInitialized = HI_FALSE;
    memset(&g_stSdInfo, 0, sizeof(g_stSdInfo));
}

HI_S32 sdcard_mount(const char *pszDevice, const char *pszFsType)
{
    const char *fsType = pszFsType ? pszFsType : "vfat";

    if (HI_NULL == pszDevice) {
        return HI_FAILURE;
    }

    if (g_stSdInfo.bMounted) {
        HI_LOG_W("SD card already mounted");
        return HI_SUCCESS;
    }

    if (mount(pszDevice, g_stSdInfo.szMountPoint, fsType,
              MS_NOATIME | MS_NODIRATIME, "iocharset=utf8") != 0) {
        HI_LOG_E("mount %s -> %s (%s) failed: %s",
                 pszDevice, g_stSdInfo.szMountPoint, fsType, strerror(errno));
        return HI_FAILURE;
    }

    strncpy(g_stSdInfo.szDevice, pszDevice, sizeof(g_stSdInfo.szDevice) - 1);
    g_stSdInfo.bMounted  = HI_TRUE;
    g_stSdInfo.bWritable = HI_TRUE;
    g_stSdInfo.bHealthy  = HI_TRUE;
    update_space_info();

    HI_LOG_I("SD card mounted: %s -> %s total=%lluMB free=%lluMB",
             pszDevice, g_stSdInfo.szMountPoint,
             (unsigned long long)g_stSdInfo.u64TotalSizeMb,
             (unsigned long long)g_stSdInfo.u64FreeSizeMb);
    return HI_SUCCESS;
}

HI_S32 sdcard_unmount(void)
{
    if (!g_stSdInfo.bMounted) {
        return HI_SUCCESS;
    }

    if (umount(g_stSdInfo.szMountPoint) != 0) {
        HI_LOG_E("umount %s failed: %s", g_stSdInfo.szMountPoint, strerror(errno));
        return HI_FAILURE;
    }

    g_stSdInfo.bMounted  = HI_FALSE;
    g_stSdInfo.bWritable = HI_FALSE;
    HI_LOG_I("SD card unmounted: %s", g_stSdInfo.szMountPoint);
    return HI_SUCCESS;
}

HI_S32 sdcard_check(void)
{
    HI_BOOL bNowMounted;

    if (!g_bInitialized) {
        return HI_FAILURE;
    }

    bNowMounted = check_mounted(g_stSdInfo.szMountPoint);

    if (!bNowMounted && g_stSdInfo.bMounted) {
        HI_LOG_W("SD card was removed unexpectedly");
        g_stSdInfo.bMounted  = HI_FALSE;
        g_stSdInfo.bWritable = HI_FALSE;
        g_stSdInfo.bHealthy  = HI_FALSE;
    } else if (bNowMounted) {
        g_stSdInfo.bMounted = HI_TRUE;
        update_space_info();
    }

    return HI_SUCCESS;
}

HI_S32 sdcard_get_info(SD_CARD_INFO_S *pstInfo)
{
    if (HI_NULL == pstInfo) {
        return HI_FAILURE;
    }
    memcpy(pstInfo, &g_stSdInfo, sizeof(SD_CARD_INFO_S));
    return HI_SUCCESS;
}

HI_BOOL sdcard_is_available(void)
{
    return (g_stSdInfo.bMounted && g_stSdInfo.bWritable && g_stSdInfo.bHealthy)
           ? HI_TRUE : HI_FALSE;
}
