#include "mpp_venc.h"
#include "hi_log.h"
#include "hi_common.h"
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#ifdef HISI_SDK_AVAILABLE
#include "hi_mpi_venc.h"
#endif

#define VENC_POLL_TIMEOUT_MS  40
#define VENC_CHN_COUNT        2

typedef struct {
    HI_BOOL         bUsed;
    HI_S32          s32Chn;
    venc_frame_cb_t pfnCb;
    void           *pPrivData;
    pthread_t       tid;
    HI_BOOL         bRunning;
    STREAM_CONF_S   stConf;
} VENC_CHN_CTX_S;

static VENC_CHN_CTX_S g_astChnCtx[VENC_CHN_COUNT];
static pthread_mutex_t g_vencMutex = PTHREAD_MUTEX_INITIALIZER;

#ifdef HISI_SDK_AVAILABLE
static HI_S32 mpp_venc_create_h264_chn(HI_S32 s32Chn, const STREAM_CONF_S *pstConf)
{
    VENC_CHN_ATTR_S stChnAttr;
    HI_S32 s32Ret;

    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.stVencAttr.enType              = PT_H264;
    stChnAttr.stVencAttr.u32MaxPicWidth      = pstConf->u32Width;
    stChnAttr.stVencAttr.u32MaxPicHeight     = pstConf->u32Height;
    stChnAttr.stVencAttr.u32BufSize          = VENC_STREAM_BUF_SIZE;
    stChnAttr.stVencAttr.u32Profile          = pstConf->u32Profile;
    stChnAttr.stVencAttr.bByFrame            = HI_TRUE;
    stChnAttr.stVencAttr.u32PicWidth         = pstConf->u32Width;
    stChnAttr.stVencAttr.u32PicHeight        = pstConf->u32Height;

    if (pstConf->enRcMode == RC_MODE_CBR) {
        VENC_H264_CBR_S stCbr;
        memset(&stCbr, 0, sizeof(stCbr));
        stCbr.u32Gop           = pstConf->u32Gop;
        stCbr.u32StatTime      = 1;
        stCbr.u32SrcFrameRate  = pstConf->u32FrameRate;
        stCbr.fr32DstFrameRate = pstConf->u32FrameRate;
        stCbr.u32BitRate       = pstConf->u32BitRate;
        stChnAttr.stRcAttr.enRcMode          = VENC_RC_MODE_H264CBR;
        stChnAttr.stRcAttr.stH264Cbr         = stCbr;
    } else {
        VENC_H264_VBR_S stVbr;
        memset(&stVbr, 0, sizeof(stVbr));
        stVbr.u32Gop           = pstConf->u32Gop;
        stVbr.u32StatTime      = 1;
        stVbr.u32SrcFrameRate  = pstConf->u32FrameRate;
        stVbr.fr32DstFrameRate = pstConf->u32FrameRate;
        stVbr.u32MaxBitRate    = pstConf->u32BitRate;
        stChnAttr.stRcAttr.enRcMode          = VENC_RC_MODE_H264VBR;
        stChnAttr.stRcAttr.stH264Vbr         = stVbr;
    }

    stChnAttr.stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    stChnAttr.stGopAttr.stNormalP.s32IPQpDelta = 2;

    s32Ret = HI_MPI_VENC_CreateChn(s32Chn, &stChnAttr);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("HI_MPI_VENC_CreateChn(%d) H264 failed: 0x%x", s32Chn, s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}

static HI_S32 mpp_venc_create_h265_chn(HI_S32 s32Chn, const STREAM_CONF_S *pstConf)
{
    VENC_CHN_ATTR_S stChnAttr;
    HI_S32 s32Ret;

    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.stVencAttr.enType              = PT_H265;
    stChnAttr.stVencAttr.u32MaxPicWidth      = pstConf->u32Width;
    stChnAttr.stVencAttr.u32MaxPicHeight     = pstConf->u32Height;
    stChnAttr.stVencAttr.u32BufSize          = VENC_STREAM_BUF_SIZE;
    stChnAttr.stVencAttr.u32Profile          = 0;
    stChnAttr.stVencAttr.bByFrame            = HI_TRUE;
    stChnAttr.stVencAttr.u32PicWidth         = pstConf->u32Width;
    stChnAttr.stVencAttr.u32PicHeight        = pstConf->u32Height;

    VENC_H265_CBR_S stCbr;
    memset(&stCbr, 0, sizeof(stCbr));
    stCbr.u32Gop           = pstConf->u32Gop;
    stCbr.u32StatTime      = 1;
    stCbr.u32SrcFrameRate  = pstConf->u32FrameRate;
    stCbr.fr32DstFrameRate = pstConf->u32FrameRate;
    stCbr.u32BitRate       = pstConf->u32BitRate;
    stChnAttr.stRcAttr.enRcMode     = VENC_RC_MODE_H265CBR;
    stChnAttr.stRcAttr.stH265Cbr   = stCbr;

    stChnAttr.stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    stChnAttr.stGopAttr.stNormalP.s32IPQpDelta = 2;

    s32Ret = HI_MPI_VENC_CreateChn(s32Chn, &stChnAttr);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("HI_MPI_VENC_CreateChn(%d) H265 failed: 0x%x", s32Chn, s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}
#endif /* HISI_SDK_AVAILABLE */

static void* venc_stream_thread(void *pArg)
{
    VENC_CHN_CTX_S *pstCtx = (VENC_CHN_CTX_S *)pArg;
#ifdef HISI_SDK_AVAILABLE
    VENC_STREAM_S   stStream;
    VENC_CHN_STATUS_S stChnStat;
    VENC_PACK_S    *pstPacks   = HI_NULL;
    HI_U32          u32PackCnt = 0;
    HI_S32          s32Ret;
    VENC_FRAME_S    stFrame;
    HI_U32          i;

    pstPacks = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * VENC_MAX_NALU_COUNT);
    if (HI_NULL == pstPacks) {
        HI_LOG_E("malloc packs failed");
        return HI_NULL;
    }

    while (pstCtx->bRunning) {
        fd_set read_fds;
        struct timeval tv;
        HI_S32 fd = HI_MPI_VENC_GetFd(pstCtx->s32Chn);

        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
        tv.tv_sec  = 0;
        tv.tv_usec = VENC_POLL_TIMEOUT_MS * 1000;

        s32Ret = select(fd + 1, &read_fds, HI_NULL, HI_NULL, &tv);
        if (s32Ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            HI_LOG_E("venc select error: %s", strerror(errno));
            break;
        }
        if (s32Ret == 0) {
            continue;
        }

        s32Ret = HI_MPI_VENC_QueryStatus(pstCtx->s32Chn, &stChnStat);
        if (HI_SUCCESS != s32Ret || stChnStat.u32CurPacks == 0) {
            continue;
        }

        u32PackCnt = (stChnStat.u32CurPacks < VENC_MAX_NALU_COUNT)
                     ? stChnStat.u32CurPacks : VENC_MAX_NALU_COUNT;

        memset(&stStream, 0, sizeof(stStream));
        stStream.pstPack   = pstPacks;
        stStream.u32PackCount = u32PackCnt;

        s32Ret = HI_MPI_VENC_GetStream(pstCtx->s32Chn, &stStream, HI_TRUE);
        if (HI_SUCCESS != s32Ret) {
            HI_LOG_E("GetStream chn%d failed: 0x%x", pstCtx->s32Chn, s32Ret);
            continue;
        }

        if (pstCtx->pfnCb != HI_NULL) {
            for (i = 0; i < stStream.u32PackCount; i++) {
                VENC_PACK_S *pPack = &stStream.pstPack[i];
                memset(&stFrame, 0, sizeof(stFrame));
                stFrame.pData     = (HI_U8 *)(HI_UL)pPack->u64PhyAddr;
                stFrame.u32Len    = pPack->u32Len - pPack->u32Offset;
                stFrame.pData    += pPack->u32Offset;
                stFrame.u64Pts   = pPack->u64PTS;
                stFrame.bKeyFrame = (pPack->DataType.enH264EType == H264E_NALU_IDRSLICE
                    || pPack->DataType.enH265EType == H265E_NALU_ISLICE) ? HI_TRUE : HI_FALSE;
                stFrame.u32NaluType = (HI_U32)pPack->DataType.enH264EType;
                pstCtx->pfnCb(pstCtx->s32Chn, &stFrame, pstCtx->pPrivData);
            }
        }

        HI_MPI_VENC_ReleaseStream(pstCtx->s32Chn, &stStream);
    }

    free(pstPacks);
#else
    HI_UNUSED(pstCtx);
    while (pstCtx->bRunning) {
        usleep(40000);
    }
#endif
    return HI_NULL;
}

static HI_S32 mpp_venc_create_chn(HI_S32 s32Chn, const STREAM_CONF_S *pstConf)
{
#ifdef HISI_SDK_AVAILABLE
    HI_S32 s32Ret;
    if (pstConf->enCodec == VENC_TYPE_H265) {
        s32Ret = mpp_venc_create_h265_chn(s32Chn, pstConf);
    } else {
        s32Ret = mpp_venc_create_h264_chn(s32Chn, pstConf);
    }
    if (HI_SUCCESS != s32Ret) {
        return s32Ret;
    }

    s32Ret = HI_MPI_VENC_StartRecvFrame(s32Chn, HI_NULL);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("HI_MPI_VENC_StartRecvFrame(%d) failed: 0x%x", s32Chn, s32Ret);
        HI_MPI_VENC_DestroyChn(s32Chn);
        return s32Ret;
    }
    return HI_SUCCESS;
#else
    HI_UNUSED(s32Chn);
    HI_UNUSED(pstConf);
    return HI_SUCCESS;
#endif
}

HI_S32 mpp_venc_init(const STREAM_CONF_S *pstMain, const STREAM_CONF_S *pstSub)
{
    HI_S32 s32Ret;

    if (HI_NULL == pstMain || HI_NULL == pstSub) {
        return HI_FAILURE;
    }

    memset(g_astChnCtx, 0, sizeof(g_astChnCtx));

    s32Ret = mpp_venc_create_chn(VENC_CHN_MAIN, pstMain);
    if (HI_SUCCESS != s32Ret) {
        return s32Ret;
    }
    g_astChnCtx[VENC_CHN_MAIN].bUsed  = HI_TRUE;
    g_astChnCtx[VENC_CHN_MAIN].s32Chn = VENC_CHN_MAIN;
    memcpy(&g_astChnCtx[VENC_CHN_MAIN].stConf, pstMain, sizeof(STREAM_CONF_S));

    s32Ret = mpp_venc_create_chn(VENC_CHN_SUB, pstSub);
    if (HI_SUCCESS != s32Ret) {
#ifdef HISI_SDK_AVAILABLE
        HI_MPI_VENC_StopRecvFrame(VENC_CHN_MAIN);
        HI_MPI_VENC_DestroyChn(VENC_CHN_MAIN);
#endif
        return s32Ret;
    }
    g_astChnCtx[VENC_CHN_SUB].bUsed  = HI_TRUE;
    g_astChnCtx[VENC_CHN_SUB].s32Chn = VENC_CHN_SUB;
    memcpy(&g_astChnCtx[VENC_CHN_SUB].stConf, pstSub, sizeof(STREAM_CONF_S));

    for (HI_S32 i = 0; i < VENC_CHN_COUNT; i++) {
        g_astChnCtx[i].bRunning = HI_TRUE;
        if (pthread_create(&g_astChnCtx[i].tid, HI_NULL,
                           venc_stream_thread, &g_astChnCtx[i]) != 0) {
            HI_LOG_E("create venc thread chn%d failed", i);
        }
    }

    HI_LOG_I("VENC initialized: main=%ux%u@%ukbps sub=%ux%u@%ukbps",
             pstMain->u32Width, pstMain->u32Height, pstMain->u32BitRate,
             pstSub->u32Width,  pstSub->u32Height,  pstSub->u32BitRate);
    return HI_SUCCESS;
}

HI_VOID mpp_venc_deinit(void)
{
    HI_S32 i;

    for (i = 0; i < VENC_CHN_COUNT; i++) {
        if (g_astChnCtx[i].bUsed) {
            g_astChnCtx[i].bRunning = HI_FALSE;
            pthread_join(g_astChnCtx[i].tid, HI_NULL);
        }
    }

#ifdef HISI_SDK_AVAILABLE
    for (i = 0; i < VENC_CHN_COUNT; i++) {
        if (g_astChnCtx[i].bUsed) {
            HI_MPI_VENC_StopRecvFrame(i);
            HI_MPI_VENC_DestroyChn(i);
        }
    }
#endif

    memset(g_astChnCtx, 0, sizeof(g_astChnCtx));
    HI_LOG_I("VENC de-initialized");
}

HI_S32 mpp_venc_register_callback(HI_S32 s32Chn, venc_frame_cb_t pfnCb,
                                   void *pPrivData)
{
    if (s32Chn < 0 || s32Chn >= VENC_CHN_COUNT) {
        return HI_FAILURE;
    }

    pthread_mutex_lock(&g_vencMutex);
    g_astChnCtx[s32Chn].pfnCb     = pfnCb;
    g_astChnCtx[s32Chn].pPrivData = pPrivData;
    pthread_mutex_unlock(&g_vencMutex);
    return HI_SUCCESS;
}

HI_S32 mpp_venc_unregister_callback(HI_S32 s32Chn)
{
    if (s32Chn < 0 || s32Chn >= VENC_CHN_COUNT) {
        return HI_FAILURE;
    }

    pthread_mutex_lock(&g_vencMutex);
    g_astChnCtx[s32Chn].pfnCb     = HI_NULL;
    g_astChnCtx[s32Chn].pPrivData = HI_NULL;
    pthread_mutex_unlock(&g_vencMutex);
    return HI_SUCCESS;
}

HI_S32 mpp_venc_request_idr(HI_S32 s32Chn)
{
    if (s32Chn < 0 || s32Chn >= VENC_CHN_COUNT) {
        return HI_FAILURE;
    }

#ifdef HISI_SDK_AVAILABLE
    HI_S32 s32Ret = HI_MPI_VENC_RequestIdr(s32Chn, HI_TRUE);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("RequestIdr chn%d failed: 0x%x", s32Chn, s32Ret);
        return s32Ret;
    }
#endif
    return HI_SUCCESS;
}

HI_S32 mpp_venc_set_bitrate(HI_S32 s32Chn, HI_U32 u32BitRate)
{
    if (s32Chn < 0 || s32Chn >= VENC_CHN_COUNT || u32BitRate == 0) {
        return HI_FAILURE;
    }

#ifdef HISI_SDK_AVAILABLE
    VENC_CHN_ATTR_S stAttr;
    HI_S32 s32Ret = HI_MPI_VENC_GetChnAttr(s32Chn, &stAttr);
    if (HI_SUCCESS != s32Ret) {
        return s32Ret;
    }
    if (stAttr.stRcAttr.enRcMode == VENC_RC_MODE_H264CBR) {
        stAttr.stRcAttr.stH264Cbr.u32BitRate = u32BitRate;
    } else if (stAttr.stRcAttr.enRcMode == VENC_RC_MODE_H265CBR) {
        stAttr.stRcAttr.stH265Cbr.u32BitRate = u32BitRate;
    }
    s32Ret = HI_MPI_VENC_SetChnAttr(s32Chn, &stAttr);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("SetChnAttr bitrate chn%d failed: 0x%x", s32Chn, s32Ret);
        return s32Ret;
    }
#endif

    g_astChnCtx[s32Chn].stConf.u32BitRate = u32BitRate;
    return HI_SUCCESS;
}
