#include "motion_detect.h"
#include "mpp_vda.h"
#include "hi_log.h"
#include "hi_utils.h"
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define MOTION_DEBOUNCE_MS   500
#define MOTION_HOLD_MS       3000

typedef struct {
    MOTION_CONF_S      stConf;
    motion_alarm_cb_t  pfnCb;
    void              *pPriv;
    HI_BOOL            bRunning;
    HI_BOOL            bAlarming;
    HI_U64             u64AlarmStartMs;
    HI_U64             u64LastTriggerMs;
    pthread_mutex_t    mutex;
    HI_BOOL            bInitialized;
} MOTION_CTX_S;

static MOTION_CTX_S g_stMotionCtx;

static void on_vda_result(HI_S32 s32Chn, HI_BOOL bAlarm, void *pPriv)
{
    MOTION_CTX_S *pstCtx = (MOTION_CTX_S *)pPriv;
    HI_U64 u64NowMs = hi_get_timestamp_ms();
    HI_BOOL bOldAlarm;

    if (!pstCtx->bRunning) {
        return;
    }

    pthread_mutex_lock(&pstCtx->mutex);
    bOldAlarm = pstCtx->bAlarming;

    if (bAlarm) {
        pstCtx->u64LastTriggerMs = u64NowMs;
        if (!pstCtx->bAlarming) {
            pstCtx->bAlarming       = HI_TRUE;
            pstCtx->u64AlarmStartMs = u64NowMs;
            HI_LOG_I("Motion detected on chn %d", s32Chn);
        }
    } else {
        if (pstCtx->bAlarming &&
            (u64NowMs - pstCtx->u64LastTriggerMs) > MOTION_HOLD_MS) {
            pstCtx->bAlarming = HI_FALSE;
            HI_LOG_I("Motion ended on chn %d", s32Chn);
        }
    }

    if (pstCtx->bAlarming != bOldAlarm && pstCtx->pfnCb != HI_NULL) {
        pstCtx->pfnCb(0, pstCtx->bAlarming, pstCtx->pPriv);
    }

    pthread_mutex_unlock(&pstCtx->mutex);
}

HI_S32 motion_detect_init(const MOTION_CONF_S *pstConf)
{
    if (HI_NULL == pstConf) {
        return HI_FAILURE;
    }

    memset(&g_stMotionCtx, 0, sizeof(g_stMotionCtx));
    memcpy(&g_stMotionCtx.stConf, pstConf, sizeof(MOTION_CONF_S));
    pthread_mutex_init(&g_stMotionCtx.mutex, HI_NULL);

    mpp_vda_register_callback(on_vda_result, &g_stMotionCtx);

    g_stMotionCtx.bInitialized = HI_TRUE;
    HI_LOG_I("Motion detect initialized, regions=%u", pstConf->u32RegionCnt);
    return HI_SUCCESS;
}

HI_VOID motion_detect_deinit(void)
{
    if (!g_stMotionCtx.bInitialized) {
        return;
    }

    motion_detect_stop();
    mpp_vda_register_callback(HI_NULL, HI_NULL);
    pthread_mutex_destroy(&g_stMotionCtx.mutex);
    memset(&g_stMotionCtx, 0, sizeof(g_stMotionCtx));
    HI_LOG_I("Motion detect de-initialized");
}

HI_S32 motion_detect_start(void)
{
    if (!g_stMotionCtx.bInitialized) {
        return HI_FAILURE;
    }
    g_stMotionCtx.bRunning = HI_TRUE;
    HI_LOG_I("Motion detect started");
    return HI_SUCCESS;
}

HI_VOID motion_detect_stop(void)
{
    g_stMotionCtx.bRunning = HI_FALSE;
    HI_LOG_I("Motion detect stopped");
}

HI_S32 motion_detect_set_region(HI_U32 u32Idx,
                                 const MOTION_REGION_CONF_S *pstRegion)
{
    if (u32Idx >= MAX_MOTION_REGIONS || HI_NULL == pstRegion) {
        return HI_FAILURE;
    }

    pthread_mutex_lock(&g_stMotionCtx.mutex);
    memcpy(&g_stMotionCtx.stConf.astRegions[u32Idx], pstRegion,
           sizeof(MOTION_REGION_CONF_S));
    if (u32Idx >= g_stMotionCtx.stConf.u32RegionCnt) {
        g_stMotionCtx.stConf.u32RegionCnt = u32Idx + 1;
    }
    pthread_mutex_unlock(&g_stMotionCtx.mutex);
    return HI_SUCCESS;
}

HI_S32 motion_detect_get_region(HI_U32 u32Idx,
                                 MOTION_REGION_CONF_S *pstRegion)
{
    if (u32Idx >= MAX_MOTION_REGIONS || HI_NULL == pstRegion) {
        return HI_FAILURE;
    }

    pthread_mutex_lock(&g_stMotionCtx.mutex);
    memcpy(pstRegion, &g_stMotionCtx.stConf.astRegions[u32Idx],
           sizeof(MOTION_REGION_CONF_S));
    pthread_mutex_unlock(&g_stMotionCtx.mutex);
    return HI_SUCCESS;
}

HI_S32 motion_detect_register_callback(motion_alarm_cb_t pfnCb, void *pPriv)
{
    pthread_mutex_lock(&g_stMotionCtx.mutex);
    g_stMotionCtx.pfnCb = pfnCb;
    g_stMotionCtx.pPriv = pPriv;
    pthread_mutex_unlock(&g_stMotionCtx.mutex);
    return HI_SUCCESS;
}

HI_BOOL motion_detect_is_alarming(void)
{
    HI_BOOL bAlarm;
    pthread_mutex_lock(&g_stMotionCtx.mutex);
    bAlarm = g_stMotionCtx.bAlarming;
    pthread_mutex_unlock(&g_stMotionCtx.mutex);
    return bAlarm;
}
