#include "config_parser.h"
#include "hi_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define CFG_LINE_MAX    512
#define CFG_SEC_MAX     64
#define CFG_KEY_MAX     64
#define CFG_VAL_MAX     256

typedef struct {
    char szSection[CFG_SEC_MAX];
    char szKey[CFG_KEY_MAX];
    char szValue[CFG_VAL_MAX];
} CFG_ENTRY_S;

#define CFG_ENTRIES_MAX  256

static CFG_ENTRY_S g_astEntries[CFG_ENTRIES_MAX];
static HI_U32      g_u32EntryCnt = 0;

static char *trim(char *str)
{
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == '\0') return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    return str;
}

static HI_S32 cfg_load(const char *pszFile)
{
    FILE *fp;
    char  szLine[CFG_LINE_MAX];
    char  szSection[CFG_SEC_MAX] = "";
    char *p;
    char *eq;

    g_u32EntryCnt = 0;

    fp = fopen(pszFile, "r");
    if (HI_NULL == fp) {
        return HI_FAILURE;
    }

    while (fgets(szLine, sizeof(szLine), fp)) {
        p = trim(szLine);
        if (*p == '\0' || *p == '#' || *p == ';') {
            continue;
        }

        if (*p == '[') {
            char *end = strchr(p, ']');
            if (end) {
                *end = '\0';
                strncpy(szSection, p + 1, sizeof(szSection) - 1);
                szSection[sizeof(szSection) - 1] = '\0';
            }
            continue;
        }

        eq = strchr(p, '=');
        if (!eq) {
            continue;
        }

        *eq = '\0';
        char *key = trim(p);
        char *val = trim(eq + 1);

        char *comment = strchr(val, '#');
        if (!comment) {
            comment = strchr(val, ';');
        }
        if (comment) {
            *comment = '\0';
            val = trim(val);
        }

        if (g_u32EntryCnt < CFG_ENTRIES_MAX) {
            CFG_ENTRY_S *e = &g_astEntries[g_u32EntryCnt++];
            strncpy(e->szSection, szSection, sizeof(e->szSection) - 1);
            strncpy(e->szKey,     key,       sizeof(e->szKey) - 1);
            strncpy(e->szValue,   val,       sizeof(e->szValue) - 1);
        }
    }

    fclose(fp);
    return HI_SUCCESS;
}

static const char *cfg_get(const char *pszSection, const char *pszKey,
                             const char *pszDefault)
{
    HI_U32 i;
    for (i = 0; i < g_u32EntryCnt; i++) {
        if (strcmp(g_astEntries[i].szSection, pszSection) == 0 &&
            strcmp(g_astEntries[i].szKey, pszKey) == 0) {
            return g_astEntries[i].szValue;
        }
    }
    return pszDefault;
}

static HI_S32 cfg_get_int(const char *sec, const char *key, HI_S32 def)
{
    const char *val = cfg_get(sec, key, HI_NULL);
    return val ? (HI_S32)strtol(val, HI_NULL, 10) : def;
}

static const char *cfg_get_str(const char *sec, const char *key,
                                const char *def)
{
    const char *val = cfg_get(sec, key, HI_NULL);
    return val ? val : def;
}

HI_VOID ipcamera_conf_set_default(IPCAMERA_CONF_S *pstConf)
{
    if (HI_NULL == pstConf) {
        return;
    }

    memset(pstConf, 0, sizeof(IPCAMERA_CONF_S));

    pstConf->video.stMain.u32Width     = 1920;
    pstConf->video.stMain.u32Height    = 1080;
    pstConf->video.stMain.u32BitRate   = 4096;
    pstConf->video.stMain.u32FrameRate = 25;
    pstConf->video.stMain.u32Gop       = 50;
    pstConf->video.stMain.enCodec      = VENC_TYPE_H264;
    pstConf->video.stMain.enRcMode     = RC_MODE_CBR;
    pstConf->video.stMain.u32Profile   = 1;

    pstConf->video.stSub.u32Width      = 1280;
    pstConf->video.stSub.u32Height     = 720;
    pstConf->video.stSub.u32BitRate    = 1024;
    pstConf->video.stSub.u32FrameRate  = 25;
    pstConf->video.stSub.u32Gop        = 50;
    pstConf->video.stSub.enCodec       = VENC_TYPE_H264;
    pstConf->video.stSub.enRcMode      = RC_MODE_CBR;
    pstConf->video.stSub.u32Profile    = 1;

    pstConf->video.u32SensorFps        = 30;

    strncpy(pstConf->network.szIp,      "192.168.1.100", 15);
    strncpy(pstConf->network.szNetmask, "255.255.255.0", 15);
    strncpy(pstConf->network.szGateway, "192.168.1.1",   15);
    pstConf->network.u16RtspPort        = 554;
    pstConf->network.bRtspAuth          = HI_FALSE;

    pstConf->motion.bEnable             = HI_TRUE;
    pstConf->motion.u32RegionCnt        = 1;
    pstConf->motion.astRegions[0].s32X  = 0;
    pstConf->motion.astRegions[0].s32Y  = 0;
    pstConf->motion.astRegions[0].u32Width  = 1920;
    pstConf->motion.astRegions[0].u32Height = 1080;
    pstConf->motion.astRegions[0].bEnable   = HI_TRUE;
    pstConf->motion.astRegions[0].u32Sensitivity = 5;
    pstConf->motion.astRegions[0].u32Threshold   = 40;

    pstConf->record.bEnable             = HI_TRUE;
    pstConf->record.enMode              = RECORD_MODE_CONTINUOUS;
    pstConf->record.u32SliceTime        = 10;
    pstConf->record.u32MaxDays          = 7;
    pstConf->record.u32PreRecord        = 5;
    pstConf->record.u32PostRecord       = 30;
    pstConf->record.u64MinFreeSpaceMb   = 256;
    strncpy(pstConf->record.szMountPoint, "/mnt/sdcard/record",
            sizeof(pstConf->record.szMountPoint) - 1);
}

HI_S32 config_parse(const char *pszFile, IPCAMERA_CONF_S *pstConf)
{
    const char *val;
    HI_S32      s32Ret;

    if (HI_NULL == pstConf) {
        return HI_FAILURE;
    }

    ipcamera_conf_set_default(pstConf);

    if (HI_NULL == pszFile) {
        return HI_SUCCESS;
    }

    s32Ret = cfg_load(pszFile);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_W("Config file not found: %s, using defaults", pszFile);
        return HI_SUCCESS;
    }

    pstConf->video.stMain.u32Width     = (HI_U32)cfg_get_int("video", "main_width",  1920);
    pstConf->video.stMain.u32Height    = (HI_U32)cfg_get_int("video", "main_height", 1080);
    pstConf->video.stMain.u32BitRate   = (HI_U32)cfg_get_int("video", "main_bitrate", 4096);
    pstConf->video.stMain.u32FrameRate = (HI_U32)cfg_get_int("video", "frame_rate",   25);
    pstConf->video.stMain.u32Gop       = (HI_U32)cfg_get_int("video", "gop",          50);

    pstConf->video.stSub.u32Width      = (HI_U32)cfg_get_int("video", "sub_width",   1280);
    pstConf->video.stSub.u32Height     = (HI_U32)cfg_get_int("video", "sub_height",   720);
    pstConf->video.stSub.u32BitRate    = (HI_U32)cfg_get_int("video", "sub_bitrate", 1024);
    pstConf->video.stSub.u32FrameRate  = pstConf->video.stMain.u32FrameRate;
    pstConf->video.stSub.u32Gop        = pstConf->video.stMain.u32Gop;
    pstConf->video.u32SensorFps        = (HI_U32)cfg_get_int("video", "sensor_fps", 30);

    val = cfg_get_str("video", "codec", "h264");
    if (strcmp(val, "h265") == 0 || strcmp(val, "hevc") == 0) {
        pstConf->video.stMain.enCodec = VENC_TYPE_H265;
        pstConf->video.stSub.enCodec  = VENC_TYPE_H265;
    }

    val = cfg_get_str("video", "rc_mode", "cbr");
    if (strcmp(val, "vbr") == 0) {
        pstConf->video.stMain.enRcMode = RC_MODE_VBR;
        pstConf->video.stSub.enRcMode  = RC_MODE_VBR;
    }

    val = cfg_get_str("network", "ip", "192.168.1.100");
    strncpy(pstConf->network.szIp, val, sizeof(pstConf->network.szIp) - 1);
    val = cfg_get_str("network", "netmask", "255.255.255.0");
    strncpy(pstConf->network.szNetmask, val, sizeof(pstConf->network.szNetmask) - 1);
    val = cfg_get_str("network", "gateway", "192.168.1.1");
    strncpy(pstConf->network.szGateway, val, sizeof(pstConf->network.szGateway) - 1);
    pstConf->network.u16RtspPort = (HI_U16)cfg_get_int("network", "rtsp_port", 554);
    pstConf->network.bRtspAuth   = (HI_BOOL)cfg_get_int("network", "rtsp_auth", 0);
    val = cfg_get_str("network", "rtsp_user", "");
    strncpy(pstConf->network.szRtspUser, val, sizeof(pstConf->network.szRtspUser) - 1);
    val = cfg_get_str("network", "rtsp_pass", "");
    strncpy(pstConf->network.szRtspPass, val, sizeof(pstConf->network.szRtspPass) - 1);

    pstConf->motion.bEnable      = (HI_BOOL)cfg_get_int("motion", "enable", 1);
    pstConf->motion.u32RegionCnt = (HI_U32)cfg_get_int("motion", "region_count", 1);
    if (pstConf->motion.u32RegionCnt > MAX_MOTION_REGIONS) {
        pstConf->motion.u32RegionCnt = MAX_MOTION_REGIONS;
    }

    HI_U32 i;
    char   szKey[32];
    for (i = 0; i < pstConf->motion.u32RegionCnt; i++) {
        snprintf(szKey, sizeof(szKey), "region_%u_x", i);
        pstConf->motion.astRegions[i].s32X = cfg_get_int("motion", szKey, 0);
        snprintf(szKey, sizeof(szKey), "region_%u_y", i);
        pstConf->motion.astRegions[i].s32Y = cfg_get_int("motion", szKey, 0);
        snprintf(szKey, sizeof(szKey), "region_%u_w", i);
        pstConf->motion.astRegions[i].u32Width = (HI_U32)cfg_get_int("motion", szKey, 1920);
        snprintf(szKey, sizeof(szKey), "region_%u_h", i);
        pstConf->motion.astRegions[i].u32Height = (HI_U32)cfg_get_int("motion", szKey, 1080);
        snprintf(szKey, sizeof(szKey), "region_%u_sens", i);
        pstConf->motion.astRegions[i].u32Sensitivity = (HI_U32)cfg_get_int("motion", szKey, 5);
        snprintf(szKey, sizeof(szKey), "region_%u_thresh", i);
        pstConf->motion.astRegions[i].u32Threshold = (HI_U32)cfg_get_int("motion", szKey, 40);
        pstConf->motion.astRegions[i].bEnable = HI_TRUE;
    }

    pstConf->record.bEnable       = (HI_BOOL)cfg_get_int("record", "enable", 1);
    pstConf->record.enMode        = (RECORD_MODE_E)cfg_get_int("record", "mode", 3);
    pstConf->record.u32SliceTime  = (HI_U32)cfg_get_int("record", "slice_time", 10);
    pstConf->record.u32MaxDays    = (HI_U32)cfg_get_int("record", "max_days", 7);
    pstConf->record.u32PreRecord  = (HI_U32)cfg_get_int("record", "pre_record", 5);
    pstConf->record.u32PostRecord = (HI_U32)cfg_get_int("record", "post_record", 30);
    pstConf->record.u64MinFreeSpaceMb = (HI_U64)cfg_get_int("record", "min_free_mb", 256);
    val = cfg_get_str("record", "mount_point", "/mnt/sdcard/record");
    strncpy(pstConf->record.szMountPoint, val,
            sizeof(pstConf->record.szMountPoint) - 1);

    HI_LOG_I("Config loaded from %s", pszFile);
    return HI_SUCCESS;
}

HI_S32 config_save(const char *pszFile, const IPCAMERA_CONF_S *pstConf)
{
    FILE *fp;

    if (HI_NULL == pszFile || HI_NULL == pstConf) {
        return HI_FAILURE;
    }

    fp = fopen(pszFile, "w");
    if (HI_NULL == fp) {
        HI_LOG_E("fopen %s for write failed: %s", pszFile, strerror(errno));
        return HI_FAILURE;
    }

    fprintf(fp, "# IPCamera Configuration\n\n");

    fprintf(fp, "[video]\n");
    fprintf(fp, "main_width = %u\n",    pstConf->video.stMain.u32Width);
    fprintf(fp, "main_height = %u\n",   pstConf->video.stMain.u32Height);
    fprintf(fp, "main_bitrate = %u\n",  pstConf->video.stMain.u32BitRate);
    fprintf(fp, "sub_width = %u\n",     pstConf->video.stSub.u32Width);
    fprintf(fp, "sub_height = %u\n",    pstConf->video.stSub.u32Height);
    fprintf(fp, "sub_bitrate = %u\n",   pstConf->video.stSub.u32BitRate);
    fprintf(fp, "frame_rate = %u\n",    pstConf->video.stMain.u32FrameRate);
    fprintf(fp, "gop = %u\n",           pstConf->video.stMain.u32Gop);
    fprintf(fp, "sensor_fps = %u\n",    pstConf->video.u32SensorFps);
    fprintf(fp, "codec = %s\n",
            (pstConf->video.stMain.enCodec == VENC_TYPE_H265) ? "h265" : "h264");
    fprintf(fp, "rc_mode = %s\n",
            (pstConf->video.stMain.enRcMode == RC_MODE_VBR) ? "vbr" : "cbr");
    fprintf(fp, "\n");

    fprintf(fp, "[network]\n");
    fprintf(fp, "ip = %s\n",        pstConf->network.szIp);
    fprintf(fp, "netmask = %s\n",   pstConf->network.szNetmask);
    fprintf(fp, "gateway = %s\n",   pstConf->network.szGateway);
    fprintf(fp, "rtsp_port = %u\n", pstConf->network.u16RtspPort);
    fprintf(fp, "rtsp_auth = %d\n", pstConf->network.bRtspAuth);
    if (pstConf->network.bRtspAuth) {
        fprintf(fp, "rtsp_user = %s\n", pstConf->network.szRtspUser);
        fprintf(fp, "rtsp_pass = %s\n", pstConf->network.szRtspPass);
    }
    fprintf(fp, "\n");

    fprintf(fp, "[motion]\n");
    fprintf(fp, "enable = %d\n",       pstConf->motion.bEnable);
    fprintf(fp, "region_count = %u\n", pstConf->motion.u32RegionCnt);

    HI_U32 i;
    for (i = 0; i < pstConf->motion.u32RegionCnt; i++) {
        fprintf(fp, "region_%u_x = %d\n",      i, pstConf->motion.astRegions[i].s32X);
        fprintf(fp, "region_%u_y = %d\n",      i, pstConf->motion.astRegions[i].s32Y);
        fprintf(fp, "region_%u_w = %u\n",      i, pstConf->motion.astRegions[i].u32Width);
        fprintf(fp, "region_%u_h = %u\n",      i, pstConf->motion.astRegions[i].u32Height);
        fprintf(fp, "region_%u_sens = %u\n",   i, pstConf->motion.astRegions[i].u32Sensitivity);
        fprintf(fp, "region_%u_thresh = %u\n", i, pstConf->motion.astRegions[i].u32Threshold);
    }
    fprintf(fp, "\n");

    fprintf(fp, "[record]\n");
    fprintf(fp, "enable = %d\n",      pstConf->record.bEnable);
    fprintf(fp, "mode = %d\n",        pstConf->record.enMode);
    fprintf(fp, "slice_time = %u\n",  pstConf->record.u32SliceTime);
    fprintf(fp, "max_days = %u\n",    pstConf->record.u32MaxDays);
    fprintf(fp, "pre_record = %u\n",  pstConf->record.u32PreRecord);
    fprintf(fp, "post_record = %u\n", pstConf->record.u32PostRecord);
    fprintf(fp, "min_free_mb = %llu\n",
            (unsigned long long)pstConf->record.u64MinFreeSpaceMb);
    fprintf(fp, "mount_point = %s\n", pstConf->record.szMountPoint);
    fprintf(fp, "\n");

    fflush(fp);
    fclose(fp);
    HI_LOG_I("Config saved to %s", pszFile);
    return HI_SUCCESS;
}
