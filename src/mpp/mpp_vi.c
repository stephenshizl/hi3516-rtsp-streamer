#include "mpp_vi.h"
#include "hi_log.h"
#include "hi_common.h"
#include <string.h>
#include <stdio.h>

#ifdef HISI_SDK_AVAILABLE
#include "hi_mpi_vi.h"
#include "hi_mpi_isp.h"
#include "hi_mpi_ae.h"
#include "hi_mpi_awb.h"
#include "mpi_mipi_rx.h"
#endif

#define VI_DEV_MAIN     0

static VI_CONF_S g_stViConf;

#ifdef HISI_SDK_AVAILABLE
static HI_S32 mpp_vi_set_mipi_attr(const VI_CONF_S *pstConf)
{
    combo_dev_attr_t stComboAttr;
    HI_S32 s32Ret;

    memset(&stComboAttr, 0, sizeof(stComboAttr));
    stComboAttr.devno             = 0;
    stComboAttr.input_mode        = INPUT_MODE_MIPI;
    stComboAttr.data_rate         = MIPI_DATA_RATE_X1;
    stComboAttr.img_rect.x        = 0;
    stComboAttr.img_rect.y        = 0;
    stComboAttr.img_rect.width    = pstConf->u32Width;
    stComboAttr.img_rect.height   = pstConf->u32Height;

    stComboAttr.mipi_attr.raw_data_type     = RAW_DATA_12BIT;
    stComboAttr.mipi_attr.lane_id[0]        = 0;
    stComboAttr.mipi_attr.lane_id[1]        = 1;
    stComboAttr.mipi_attr.lane_id[2]        = 2;
    stComboAttr.mipi_attr.lane_id[3]        = 3;

    s32Ret = HI_MPI_MIPI_SetMipiAttr(0, &stComboAttr);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("HI_MPI_MIPI_SetMipiAttr failed: 0x%x", s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}

static HI_S32 mpp_vi_start_dev(const VI_CONF_S *pstConf)
{
    VI_DEV_ATTR_S stDevAttr;
    HI_S32 s32Ret;

    memset(&stDevAttr, 0, sizeof(stDevAttr));
    stDevAttr.enIntfMode    = VI_MODE_MIPI;
    stDevAttr.enWorkMode    = VI_WORK_MODE_1Multiplex;
    stDevAttr.enScanMode    = VI_SCAN_PROGRESSIVE;
    stDevAttr.s32AdChnId[0] = -1;
    stDevAttr.s32AdChnId[1] = -1;
    stDevAttr.s32AdChnId[2] = -1;
    stDevAttr.s32AdChnId[3] = -1;

    s32Ret = HI_MPI_VI_SetDevAttr(VI_DEV_MAIN, &stDevAttr);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("HI_MPI_VI_SetDevAttr failed: 0x%x", s32Ret);
        return s32Ret;
    }

    s32Ret = HI_MPI_VI_EnableDev(VI_DEV_MAIN);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("HI_MPI_VI_EnableDev failed: 0x%x", s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}

static HI_S32 mpp_vi_start_pipe(const VI_CONF_S *pstConf)
{
    VI_PIPE_ATTR_S stPipeAttr;
    HI_S32 s32Ret;

    memset(&stPipeAttr, 0, sizeof(stPipeAttr));
    stPipeAttr.enPipeBypassMode   = VI_PIPE_BYPASS_NONE;
    stPipeAttr.bIspBypass         = HI_FALSE;
    stPipeAttr.u32MaxW            = pstConf->u32Width;
    stPipeAttr.u32MaxH            = pstConf->u32Height;
    stPipeAttr.enPixFmt           = PIXEL_FORMAT_RGB_BAYER_12BPP;
    stPipeAttr.enCompressMode     = COMPRESS_MODE_NONE;
    stPipeAttr.enBitWidth         = DATA_BITWIDTH_12;

    s32Ret = HI_MPI_VI_CreatePipe(VI_PIPE_MAIN, &stPipeAttr);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("HI_MPI_VI_CreatePipe failed: 0x%x", s32Ret);
        return s32Ret;
    }

    s32Ret = HI_MPI_VI_StartPipe(VI_PIPE_MAIN);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("HI_MPI_VI_StartPipe failed: 0x%x", s32Ret);
        HI_MPI_VI_DestroyPipe(VI_PIPE_MAIN);
        return s32Ret;
    }

    return HI_SUCCESS;
}

static HI_S32 mpp_vi_start_chn(const VI_CONF_S *pstConf)
{
    VI_CHN_ATTR_S stChnAttr;
    HI_S32 s32Ret;

    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.stSize.u32Width  = pstConf->u32Width;
    stChnAttr.stSize.u32Height = pstConf->u32Height;
    stChnAttr.enPixFormat      = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    stChnAttr.enDynamicRange   = DYNAMIC_RANGE_SDR8;
    stChnAttr.enVideoFormat    = VIDEO_FORMAT_LINEAR;
    stChnAttr.enCompressMode   = COMPRESS_MODE_NONE;
    stChnAttr.bMirror          = HI_FALSE;
    stChnAttr.bFlip            = HI_FALSE;
    stChnAttr.u32Depth         = 0;
    stChnAttr.stFrameRate.s32SrcFrameRate = -1;
    stChnAttr.stFrameRate.s32DstFrameRate = -1;

    s32Ret = HI_MPI_VI_SetChnAttr(VI_PIPE_MAIN, VI_CHN_MAIN, &stChnAttr);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("HI_MPI_VI_SetChnAttr failed: 0x%x", s32Ret);
        return s32Ret;
    }

    s32Ret = HI_MPI_VI_EnableChn(VI_PIPE_MAIN, VI_CHN_MAIN);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("HI_MPI_VI_EnableChn failed: 0x%x", s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}
#endif /* HISI_SDK_AVAILABLE */

HI_S32 mpp_vi_init(const VI_CONF_S *pstConf)
{
    if (HI_NULL == pstConf) {
        return HI_FAILURE;
    }

    memcpy(&g_stViConf, pstConf, sizeof(VI_CONF_S));

#ifdef HISI_SDK_AVAILABLE
    HI_S32 s32Ret;

    if (pstConf->enViMode == VI_MODE_MIPI) {
        s32Ret = mpp_vi_set_mipi_attr(pstConf);
        if (HI_SUCCESS != s32Ret) {
            return s32Ret;
        }
    }

    s32Ret = mpp_vi_start_dev(pstConf);
    if (HI_SUCCESS != s32Ret) {
        return s32Ret;
    }

    s32Ret = mpp_vi_start_pipe(pstConf);
    if (HI_SUCCESS != s32Ret) {
        HI_MPI_VI_DisableDev(VI_DEV_MAIN);
        return s32Ret;
    }

    s32Ret = mpp_vi_start_chn(pstConf);
    if (HI_SUCCESS != s32Ret) {
        HI_MPI_VI_StopPipe(VI_PIPE_MAIN);
        HI_MPI_VI_DestroyPipe(VI_PIPE_MAIN);
        HI_MPI_VI_DisableDev(VI_DEV_MAIN);
        return s32Ret;
    }
#endif

    HI_LOG_I("VI initialized: %ux%u@%u fps mode=%d",
             pstConf->u32Width, pstConf->u32Height,
             pstConf->u32FrameRate, pstConf->enViMode);
    return HI_SUCCESS;
}

HI_VOID mpp_vi_deinit(void)
{
#ifdef HISI_SDK_AVAILABLE
    HI_MPI_VI_DisableChn(VI_PIPE_MAIN, VI_CHN_MAIN);
    HI_MPI_VI_StopPipe(VI_PIPE_MAIN);
    HI_MPI_VI_DestroyPipe(VI_PIPE_MAIN);
    HI_MPI_VI_DisableDev(VI_DEV_MAIN);
#endif
    HI_LOG_I("VI de-initialized");
}
