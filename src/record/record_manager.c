#include "record_manager.h"
#include "file_writer.h"
#include "circular_storage.h"
#include "sd_card_manager.h"
#include "hi_log.h"
#include "hi_utils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define RECORD_PRE_BUF_FRAMES   125
#define RECORD_FRAME_MAX_SIZE   (512 * 1024)

typedef enum {
    RECORD_STATE_IDLE    = 0,
    RECORD_STATE_WAITING = 1,
    RECORD_STATE_WRITING = 2,
} RECORD_STATE_E;

typedef struct {
    HI_U8  *pData;
    HI_U32  u32Len;
    HI_U64  u64Pts;
    HI_BOOL bKeyFrame;
} PRE_RECORD_FRAME_S;

typedef struct {
    RECORD_CONF_S       stConf;
    RECORD_STATE_E      enState;
    FILE_HANDLE_T       hFile;
    char                szCurFile[256];
    HI_U64              u64StartPts;
    HI_U64              u64LastSlicePts;
    HI_BOOL             bRunning;
    HI_BOOL             bInitialized;
    HI_BOOL             bMotionAlarm;
    HI_U64              u64MotionEndMs;
    pthread_mutex_t     mutex;
    PRE_RECORD_FRAME_S  astPreBuf[RECORD_PRE_BUF_FRAMES];
    HI_U32              u32PreHead;
    HI_U32              u32PreCnt;
} RECORD_CTX_S;

static RECORD_CTX_S g_stRecordCtx;

static void gen_file_path(char *pszPath, HI_U32 u32Size)
{
    struct tm  tm_info;
    time_t     tNow = time(HI_NULL);

    localtime_r(&tNow, &tm_info);
    snprintf(pszPath, u32Size,
             "%s/%04d%02d%02d_%02d%02d%02d.mp4",
             g_stRecordCtx.stConf.szMountPoint,
             tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday,
             tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec);
}

static HI_S32 open_new_file(HI_U64 u64Pts)
{
    FILE_CONF_S stFileConf;
    char        szPath[256];

    gen_file_path(szPath, sizeof(szPath));

    memset(&stFileConf, 0, sizeof(stFileConf));
    strncpy(stFileConf.szFilePath, szPath, sizeof(stFileConf.szFilePath) - 1);
    stFileConf.enFormat    = FILE_FORMAT_MP4;
    stFileConf.enCodec     = VENC_TYPE_H264;
    stFileConf.u32Width    = 1920;
    stFileConf.u32Height   = 1080;
    stFileConf.u32FrameRate = 25;
    stFileConf.u32Duration = g_stRecordCtx.stConf.u32SliceTime * 60;

    g_stRecordCtx.hFile = file_writer_open(&stFileConf);
    if (FILE_WRITER_INVALID_FD == g_stRecordCtx.hFile) {
        return HI_FAILURE;
    }

    strncpy(g_stRecordCtx.szCurFile, szPath, sizeof(g_stRecordCtx.szCurFile) - 1);
    g_stRecordCtx.u64StartPts     = u64Pts;
    g_stRecordCtx.u64LastSlicePts = u64Pts;
    HI_LOG_I("New record file: %s", szPath);
    return HI_SUCCESS;
}

static HI_S32 close_current_file(void)
{
    HI_S32 s32Ret;

    if (FILE_WRITER_INVALID_FD == g_stRecordCtx.hFile) {
        return HI_SUCCESS;
    }

    s32Ret = file_writer_close(g_stRecordCtx.hFile);
    circular_storage_add_file(g_stRecordCtx.szCurFile);
    g_stRecordCtx.hFile = FILE_WRITER_INVALID_FD;
    g_stRecordCtx.szCurFile[0] = '\0';
    return s32Ret;
}

static void flush_pre_buffer(void)
{
    HI_U32 i;
    HI_U32 u32Start;

    if (g_stRecordCtx.u32PreCnt == 0) {
        return;
    }

    if (g_stRecordCtx.u32PreCnt >= RECORD_PRE_BUF_FRAMES) {
        u32Start = g_stRecordCtx.u32PreHead;
    } else {
        u32Start = (g_stRecordCtx.u32PreHead + RECORD_PRE_BUF_FRAMES
                    - g_stRecordCtx.u32PreCnt) % RECORD_PRE_BUF_FRAMES;
    }

    for (i = 0; i < g_stRecordCtx.u32PreCnt; i++) {
        HI_U32 idx = (u32Start + i) % RECORD_PRE_BUF_FRAMES;
        PRE_RECORD_FRAME_S *pFrame = &g_stRecordCtx.astPreBuf[idx];
        if (pFrame->pData && pFrame->u32Len > 0) {
            file_writer_write_video(g_stRecordCtx.hFile,
                                    pFrame->pData, pFrame->u32Len,
                                    pFrame->u64Pts, pFrame->bKeyFrame);
        }
    }
}

static void push_pre_buffer(const HI_U8 *pData, HI_U32 u32Len,
                             HI_U64 u64Pts, HI_BOOL bKeyFrame)
{
    PRE_RECORD_FRAME_S *pFrame;
    HI_U32 u32CopyLen;

    if (u32Len == 0 || HI_NULL == pData) {
        return;
    }

    pFrame = &g_stRecordCtx.astPreBuf[g_stRecordCtx.u32PreHead];

    u32CopyLen = (u32Len < RECORD_FRAME_MAX_SIZE) ? u32Len : RECORD_FRAME_MAX_SIZE;

    if (HI_NULL == pFrame->pData) {
        pFrame->pData = (HI_U8 *)malloc(RECORD_FRAME_MAX_SIZE);
    }

    if (pFrame->pData != HI_NULL) {
        memcpy(pFrame->pData, pData, u32CopyLen);
        pFrame->u32Len   = u32CopyLen;
        pFrame->u64Pts   = u64Pts;
        pFrame->bKeyFrame = bKeyFrame;
    }

    g_stRecordCtx.u32PreHead = (g_stRecordCtx.u32PreHead + 1) % RECORD_PRE_BUF_FRAMES;
    if (g_stRecordCtx.u32PreCnt < RECORD_PRE_BUF_FRAMES) {
        g_stRecordCtx.u32PreCnt++;
    }
}

HI_S32 record_manager_init(const RECORD_CONF_S *pstConf)
{
    CIRCULAR_STORAGE_CONF_S stCsConf;

    if (HI_NULL == pstConf) {
        return HI_FAILURE;
    }

    memset(&g_stRecordCtx, 0, sizeof(g_stRecordCtx));
    memcpy(&g_stRecordCtx.stConf, pstConf, sizeof(RECORD_CONF_S));
    g_stRecordCtx.hFile = FILE_WRITER_INVALID_FD;
    pthread_mutex_init(&g_stRecordCtx.mutex, HI_NULL);

    if (g_stRecordCtx.stConf.u32SliceTime == 0) {
        g_stRecordCtx.stConf.u32SliceTime = 10;
    }
    if (g_stRecordCtx.stConf.u32MaxDays == 0) {
        g_stRecordCtx.stConf.u32MaxDays = 7;
    }

    file_writer_init();

    memset(&stCsConf, 0, sizeof(stCsConf));
    strncpy(stCsConf.szPath, pstConf->szMountPoint, sizeof(stCsConf.szPath) - 1);
    stCsConf.u64MinFreeSpaceMb  = pstConf->u64MinFreeSpaceMb;
    if (stCsConf.u64MinFreeSpaceMb == 0) {
        stCsConf.u64MinFreeSpaceMb = 256;
    }
    stCsConf.u32MaxFileAgeDays = pstConf->u32MaxDays;
    circular_storage_init(&stCsConf);

    g_stRecordCtx.bInitialized = HI_TRUE;
    HI_LOG_I("Record manager initialized: mode=%d path=%s",
             pstConf->enMode, pstConf->szMountPoint);
    return HI_SUCCESS;
}

HI_VOID record_manager_deinit(void)
{
    if (!g_stRecordCtx.bInitialized) {
        return;
    }

    record_manager_stop();

    pthread_mutex_lock(&g_stRecordCtx.mutex);

    HI_U32 i;
    for (i = 0; i < RECORD_PRE_BUF_FRAMES; i++) {
        if (g_stRecordCtx.astPreBuf[i].pData) {
            free(g_stRecordCtx.astPreBuf[i].pData);
            g_stRecordCtx.astPreBuf[i].pData = HI_NULL;
        }
    }

    pthread_mutex_unlock(&g_stRecordCtx.mutex);

    circular_storage_deinit();
    file_writer_deinit();
    pthread_mutex_destroy(&g_stRecordCtx.mutex);
    g_stRecordCtx.bInitialized = HI_FALSE;
}

HI_S32 record_manager_start(void)
{
    if (!g_stRecordCtx.bInitialized) {
        return HI_FAILURE;
    }

    g_stRecordCtx.bRunning = HI_TRUE;
    HI_LOG_I("Record manager started");
    return HI_SUCCESS;
}

HI_VOID record_manager_stop(void)
{
    if (!g_stRecordCtx.bRunning) {
        return;
    }

    pthread_mutex_lock(&g_stRecordCtx.mutex);
    close_current_file();
    g_stRecordCtx.enState  = RECORD_STATE_IDLE;
    g_stRecordCtx.bRunning = HI_FALSE;
    pthread_mutex_unlock(&g_stRecordCtx.mutex);

    HI_LOG_I("Record manager stopped");
}

HI_S32 record_manager_feed_frame(HI_S32 s32Chn, const HI_U8 *pData,
                                  HI_U32 u32Len, HI_U64 u64Pts,
                                  HI_BOOL bKeyFrame)
{
    HI_U64 u64NowMs;
    HI_U32 u32SliceUs;
    HI_BOOL bShouldRecord;

    if (!g_stRecordCtx.bRunning || HI_NULL == pData || u32Len == 0) {
        return HI_SUCCESS;
    }

    if (s32Chn != VENC_CHN_MAIN) {
        return HI_SUCCESS;
    }

    if (!sdcard_is_available()) {
        return HI_SUCCESS;
    }

    pthread_mutex_lock(&g_stRecordCtx.mutex);

    bShouldRecord = HI_FALSE;
    switch (g_stRecordCtx.stConf.enMode) {
    case RECORD_MODE_CONTINUOUS:
        bShouldRecord = HI_TRUE;
        break;
    case RECORD_MODE_MOTION:
        u64NowMs = hi_get_timestamp_ms();
        if (g_stRecordCtx.bMotionAlarm) {
            g_stRecordCtx.u64MotionEndMs = u64NowMs
                + g_stRecordCtx.stConf.u32PostRecord * 1000;
            bShouldRecord = HI_TRUE;
        } else if (u64NowMs < g_stRecordCtx.u64MotionEndMs) {
            bShouldRecord = HI_TRUE;
        }
        break;
    case RECORD_MODE_MANUAL:
        bShouldRecord = (g_stRecordCtx.enState == RECORD_STATE_WRITING);
        break;
    default:
        break;
    }

    if (!bShouldRecord) {
        if (g_stRecordCtx.stConf.enMode == RECORD_MODE_MOTION ||
            g_stRecordCtx.stConf.u32PreRecord > 0) {
            push_pre_buffer(pData, u32Len, u64Pts, bKeyFrame);
        }
        if (g_stRecordCtx.enState == RECORD_STATE_WRITING) {
            close_current_file();
            g_stRecordCtx.enState = RECORD_STATE_IDLE;
        }
        pthread_mutex_unlock(&g_stRecordCtx.mutex);
        return HI_SUCCESS;
    }

    if (g_stRecordCtx.enState != RECORD_STATE_WRITING) {
        if (!bKeyFrame) {
            push_pre_buffer(pData, u32Len, u64Pts, bKeyFrame);
            pthread_mutex_unlock(&g_stRecordCtx.mutex);
            return HI_SUCCESS;
        }
        if (open_new_file(u64Pts) != HI_SUCCESS) {
            pthread_mutex_unlock(&g_stRecordCtx.mutex);
            return HI_FAILURE;
        }
        if (g_stRecordCtx.stConf.u32PreRecord > 0) {
            flush_pre_buffer();
        }
        g_stRecordCtx.enState = RECORD_STATE_WRITING;
    }

    u32SliceUs = g_stRecordCtx.stConf.u32SliceTime * 60 * 1000000UL;
    if (bKeyFrame &&
        (u64Pts - g_stRecordCtx.u64LastSlicePts) >= u32SliceUs) {
        close_current_file();
        if (open_new_file(u64Pts) != HI_SUCCESS) {
            g_stRecordCtx.enState = RECORD_STATE_IDLE;
            pthread_mutex_unlock(&g_stRecordCtx.mutex);
            return HI_FAILURE;
        }
        g_stRecordCtx.u64LastSlicePts = u64Pts;
    }

    file_writer_write_video(g_stRecordCtx.hFile, pData, u32Len,
                            u64Pts, bKeyFrame);

    pthread_mutex_unlock(&g_stRecordCtx.mutex);
    return HI_SUCCESS;
}

HI_VOID record_manager_on_motion(HI_BOOL bAlarm)
{
    pthread_mutex_lock(&g_stRecordCtx.mutex);
    g_stRecordCtx.bMotionAlarm = bAlarm;
    if (bAlarm) {
        HI_LOG_I("Recording triggered by motion alarm");
    }
    pthread_mutex_unlock(&g_stRecordCtx.mutex);
}

HI_BOOL record_manager_is_recording(void)
{
    return (g_stRecordCtx.enState == RECORD_STATE_WRITING) ? HI_TRUE : HI_FALSE;
}
