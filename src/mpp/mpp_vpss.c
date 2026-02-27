#include "mpp_vpss.h"
#include "hi_log.h"
#include "hi_common.h"
#include <string.h>

#ifdef HISI_SDK_AVAILABLE
#include "hi_mpi_vpss.h"
#endif

#define VPSS_VDA_WIDTH   352
#define VPSS_VDA_HEIGHT  288

#ifdef HISI_SDK_AVAILABLE
static HI_S32 mpp_vpss_start_grp(HI_S32 s32Grp, const VPSS_GRP_CONF_S *pstConf)
{
    VPSS_GRP_ATTR_S stGrpAttr;
    HI_S32 s32Ret;

    memset(&stGrpAttr, 0, sizeof(stGrpAttr));
    stGrpAttr.u32MaxW              = pstConf->u32SrcWidth;
    stGrpAttr.u32MaxH              = pstConf->u32SrcHeight;
    stGrpAttr.enPixelFormat        = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    stGrpAttr.enDynamicRange       = DYNAMIC_RANGE_SDR8;
    stGrpAttr.stFrameRate.s32SrcFrameRate = -1;
    stGrpAttr.stFrameRate.s32DstFrameRate = -1;
    stGrpAttr.bNrEn                = HI_TRUE;

    s32Ret = HI_MPI_VPSS_CreateGrp(s32Grp, &stGrpAttr);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("HI_MPI_VPSS_CreateGrp(%d) failed: 0x%x", s32Grp, s32Ret);
        return s32Ret;
    }

    s32Ret = HI_MPI_VPSS_StartGrp(s32Grp);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("HI_MPI_VPSS_StartGrp(%d) failed: 0x%x", s32Grp, s32Ret);
        HI_MPI_VPSS_DestroyGrp(s32Grp);
        return s32Ret;
    }

    return HI_SUCCESS;
}

static HI_S32 mpp_vpss_enable_chn(HI_S32 s32Grp, HI_S32 s32Chn,
                                    HI_U32 u32W, HI_U32 u32H,
                                    HI_U32 u32FpsIn, HI_U32 u32FpsOut)
{
    VPSS_CHN_ATTR_S stChnAttr;
    HI_S32 s32Ret;

    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.u32Width           = u32W;
    stChnAttr.u32Height          = u32H;
    stChnAttr.enChnMode          = VPSS_CHN_MODE_USER;
    stChnAttr.enCompressMode     = COMPRESS_MODE_NONE;
    stChnAttr.enDynamicRange     = DYNAMIC_RANGE_SDR8;
    stChnAttr.enPixelFormat      = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    stChnAttr.stFrameRate.s32SrcFrameRate = (HI_S32)u32FpsIn;
    stChnAttr.stFrameRate.s32DstFrameRate = (HI_S32)u32FpsOut;
    stChnAttr.u32Depth           = 0;
    stChnAttr.bMirror            = HI_FALSE;
    stChnAttr.bFlip              = HI_FALSE;
    stChnAttr.enVideoFormat      = VIDEO_FORMAT_LINEAR;

    s32Ret = HI_MPI_VPSS_SetChnAttr(s32Grp, s32Chn, &stChnAttr);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("HI_MPI_VPSS_SetChnAttr(%d,%d) failed: 0x%x",
                 s32Grp, s32Chn, s32Ret);
        return s32Ret;
    }

    s32Ret = HI_MPI_VPSS_EnableChn(s32Grp, s32Chn);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("HI_MPI_VPSS_EnableChn(%d,%d) failed: 0x%x",
                 s32Grp, s32Chn, s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}
#endif /* HISI_SDK_AVAILABLE */

HI_S32 mpp_vpss_init(const VIDEO_CONF_S *pstVideoConf)
{
    if (HI_NULL == pstVideoConf) {
        return HI_FAILURE;
    }

#ifdef HISI_SDK_AVAILABLE
    VPSS_GRP_CONF_S stGrpConf;
    HI_S32 s32Ret;

    memset(&stGrpConf, 0, sizeof(stGrpConf));
    stGrpConf.u32SrcWidth  = pstVideoConf->stMain.u32Width;
    stGrpConf.u32SrcHeight = pstVideoConf->stMain.u32Height;

    s32Ret = mpp_vpss_start_grp(VPSS_GRP_MAIN, &stGrpConf);
    if (HI_SUCCESS != s32Ret) {
        return s32Ret;
    }

    s32Ret = mpp_vpss_enable_chn(VPSS_GRP_MAIN, VPSS_CHN_MAIN,
                                  pstVideoConf->stMain.u32Width,
                                  pstVideoConf->stMain.u32Height,
                                  pstVideoConf->u32SensorFps,
                                  pstVideoConf->stMain.u32FrameRate);
    if (HI_SUCCESS != s32Ret) {
        HI_MPI_VPSS_StopGrp(VPSS_GRP_MAIN);
        HI_MPI_VPSS_DestroyGrp(VPSS_GRP_MAIN);
        return s32Ret;
    }

    s32Ret = mpp_vpss_enable_chn(VPSS_GRP_MAIN, VPSS_CHN_SUB,
                                  pstVideoConf->stSub.u32Width,
                                  pstVideoConf->stSub.u32Height,
                                  pstVideoConf->u32SensorFps,
                                  pstVideoConf->stSub.u32FrameRate);
    if (HI_SUCCESS != s32Ret) {
        HI_MPI_VPSS_DisableChn(VPSS_GRP_MAIN, VPSS_CHN_MAIN);
        HI_MPI_VPSS_StopGrp(VPSS_GRP_MAIN);
        HI_MPI_VPSS_DestroyGrp(VPSS_GRP_MAIN);
        return s32Ret;
    }

    s32Ret = mpp_vpss_enable_chn(VPSS_GRP_MAIN, VPSS_CHN_VDA,
                                  VPSS_VDA_WIDTH, VPSS_VDA_HEIGHT,
                                  pstVideoConf->u32SensorFps, 10);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_W("VDA channel enable failed, motion detect disabled");
    }
#endif

    HI_LOG_I("VPSS initialized: main=%ux%u sub=%ux%u",
             pstVideoConf->stMain.u32Width, pstVideoConf->stMain.u32Height,
             pstVideoConf->stSub.u32Width,  pstVideoConf->stSub.u32Height);
    return HI_SUCCESS;
}

HI_VOID mpp_vpss_deinit(void)
{
#ifdef HISI_SDK_AVAILABLE
    HI_MPI_VPSS_DisableChn(VPSS_GRP_MAIN, VPSS_CHN_VDA);
    HI_MPI_VPSS_DisableChn(VPSS_GRP_MAIN, VPSS_CHN_SUB);
    HI_MPI_VPSS_DisableChn(VPSS_GRP_MAIN, VPSS_CHN_MAIN);
    HI_MPI_VPSS_StopGrp(VPSS_GRP_MAIN);
    HI_MPI_VPSS_DestroyGrp(VPSS_GRP_MAIN);
#endif
    HI_LOG_I("VPSS de-initialized");
}
