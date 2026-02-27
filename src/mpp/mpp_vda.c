#include "mpp_vda.h"
#include "hi_log.h"
#include "hi_common.h"
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#ifdef HISI_SDK_AVAILABLE
#include "hi_mpi_vda.h"
#endif

#define VDA_GRID_W      22
#define VDA_GRID_H      18
#define VDA_POLL_TIMEOUT_MS  100

typedef struct {
    HI_BOOL          bInitialized;
    MOTION_CONF_S    stConf;
    vda_result_cb_t  pfnCb;
    void            *pPriv;
    pthread_t        tid;
    HI_BOOL          bRunning;
} VDA_CTX_S;

static VDA_CTX_S g_stVdaCtx;

#ifdef HISI_SDK_AVAILABLE
static HI_BOOL vda_check_alarm(const VDA_DATA_S *pstData,
                                 const MOTION_CONF_S *pstConf)
{
    HI_U32 u32TotalSet = 0;
    HI_U32 u32GridCnt  = VDA_GRID_W * VDA_GRID_H;
    HI_U32 i;

    for (i = 0; i < u32GridCnt / 8 + 1 && i < 64; i++) {
        HI_U8 byte = (HI_U8)(pstData->unData.stMdData.au64Data[i / 8] >> ((i % 8) * 8));
        while (byte) {
            u32TotalSet += byte & 1;
            byte >>= 1;
        }
    }

    HI_U32 u32Threshold = pstConf->astRegions[0].u32Threshold;
    if (u32Threshold == 0) {
        u32Threshold = u32GridCnt / 10;
    }

    return (u32TotalSet >= u32Threshold) ? HI_TRUE : HI_FALSE;
}
#endif

static void* vda_poll_thread(void *pArg)
{
    VDA_CTX_S *pstCtx = (VDA_CTX_S *)pArg;

#ifdef HISI_SDK_AVAILABLE
    VDA_DATA_S stData;
    HI_S32     s32Ret;

    while (pstCtx->bRunning) {
        s32Ret = HI_MPI_VDA_GetData(VDA_CHN_MAIN, &stData,
                                     VDA_POLL_TIMEOUT_MS);
        if (HI_SUCCESS != s32Ret) {
            usleep(10000);
            continue;
        }

        if (pstCtx->pfnCb != HI_NULL &&
            stData.enWorkMode == VDA_WORK_MODE_MD) {
            HI_BOOL bAlarm = vda_check_alarm(&stData, &pstCtx->stConf);
            pstCtx->pfnCb(VDA_CHN_MAIN, bAlarm, pstCtx->pPriv);
        }

        HI_MPI_VDA_ReleaseData(VDA_CHN_MAIN, &stData);
    }
#else
    HI_UNUSED(pstCtx);
    while (pstCtx->bRunning) {
        usleep(100000);
    }
#endif
    return HI_NULL;
}

HI_S32 mpp_vda_init(const MOTION_CONF_S *pstConf)
{
    if (HI_NULL == pstConf) {
        return HI_FAILURE;
    }

    memset(&g_stVdaCtx, 0, sizeof(g_stVdaCtx));
    memcpy(&g_stVdaCtx.stConf, pstConf, sizeof(MOTION_CONF_S));

#ifdef HISI_SDK_AVAILABLE
    VDA_CHN_ATTR_S stAttr;
    HI_U32 i;
    HI_S32 s32Ret;

    memset(&stAttr, 0, sizeof(stAttr));
    stAttr.enWorkMode             = VDA_WORK_MODE_MD;
    stAttr.u32Width               = 352;
    stAttr.u32Height              = 288;
    stAttr.stMdAttr.enMbSize      = VDA_MB_8X8;
    stAttr.stMdAttr.enMbSadBits   = VDA_MB_SAD_8BIT;
    stAttr.stMdAttr.enRefType     = VDA_REF_DYNAMIC;
    stAttr.stMdAttr.u32BufLine    = 6;
    stAttr.stMdAttr.u32VdaGrid    = VDA_GRID_W * VDA_GRID_H;

    for (i = 0; i < VDA_GRID_W * VDA_GRID_H; i++) {
        stAttr.stMdAttr.au16Threshold[i] =
            (HI_U16)(pstConf->u32RegionCnt > 0
                     ? pstConf->astRegions[0].u32Sensitivity * 30 : 150);
    }

    s32Ret = HI_MPI_VDA_CreateChn(VDA_CHN_MAIN, &stAttr);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("HI_MPI_VDA_CreateChn failed: 0x%x", s32Ret);
        return s32Ret;
    }

    s32Ret = HI_MPI_VDA_StartRecvPic(VDA_CHN_MAIN);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("HI_MPI_VDA_StartRecvPic failed: 0x%x", s32Ret);
        HI_MPI_VDA_DestroyChn(VDA_CHN_MAIN);
        return s32Ret;
    }
#endif

    g_stVdaCtx.bRunning = HI_TRUE;
    if (pthread_create(&g_stVdaCtx.tid, HI_NULL, vda_poll_thread,
                       &g_stVdaCtx) != 0) {
        HI_LOG_E("create VDA thread failed");
#ifdef HISI_SDK_AVAILABLE
        HI_MPI_VDA_StopRecvPic(VDA_CHN_MAIN);
        HI_MPI_VDA_DestroyChn(VDA_CHN_MAIN);
#endif
        return HI_FAILURE;
    }

    g_stVdaCtx.bInitialized = HI_TRUE;
    HI_LOG_I("VDA initialized, sensitivity=%u",
             pstConf->u32RegionCnt > 0 ? pstConf->astRegions[0].u32Sensitivity : 5);
    return HI_SUCCESS;
}

HI_VOID mpp_vda_deinit(void)
{
    if (!g_stVdaCtx.bInitialized) {
        return;
    }

    g_stVdaCtx.bRunning = HI_FALSE;
    pthread_join(g_stVdaCtx.tid, HI_NULL);

#ifdef HISI_SDK_AVAILABLE
    HI_MPI_VDA_StopRecvPic(VDA_CHN_MAIN);
    HI_MPI_VDA_DestroyChn(VDA_CHN_MAIN);
#endif

    memset(&g_stVdaCtx, 0, sizeof(g_stVdaCtx));
    HI_LOG_I("VDA de-initialized");
}

HI_S32 mpp_vda_register_callback(vda_result_cb_t pfnCb, void *pPriv)
{
    g_stVdaCtx.pfnCb = pfnCb;
    g_stVdaCtx.pPriv = pPriv;
    return HI_SUCCESS;
}
