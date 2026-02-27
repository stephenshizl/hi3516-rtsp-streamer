#include "mpp_vb.h"
#include "hi_log.h"
#include "hi_common.h"
#include <string.h>

#ifdef HISI_SDK_AVAILABLE
#include "hi_mpi_vb.h"
#endif

#define VB_ALIGN   64

HI_U32 mpp_vb_calc_blk_size(HI_U32 u32Width, HI_U32 u32Height,
                              PIXEL_FORMAT_E enPixFmt, HI_U32 u32Align)
{
    HI_U32 u32AlignW = ALIGN_UP(u32Width, u32Align);
    HI_U32 u32AlignH = ALIGN_UP(u32Height, u32Align);
    HI_U32 u32Size   = 0;

    switch (enPixFmt) {
    case PIXEL_FORMAT_YVU_SEMIPLANAR_420:
        u32Size = u32AlignW * u32AlignH * 3 / 2;
        break;
    case PIXEL_FORMAT_YVU_SEMIPLANAR_422:
        u32Size = u32AlignW * u32AlignH * 2;
        break;
    default:
        u32Size = u32AlignW * u32AlignH * 2;
        break;
    }

    return u32Size;
}

HI_S32 mpp_vb_init(const IPCAMERA_CONF_S *pstConf)
{
#ifdef HISI_SDK_AVAILABLE
    VB_CONF_S stVbConf;
    HI_U32    u32BlkSize;
    HI_S32    s32Ret;

    if (HI_NULL == pstConf) {
        return HI_FAILURE;
    }

    memset(&stVbConf, 0, sizeof(stVbConf));
    stVbConf.u32MaxPoolCnt = VB_POOL_MAX_CNT;

    u32BlkSize = mpp_vb_calc_blk_size(pstConf->video.stMain.u32Width,
                                       pstConf->video.stMain.u32Height,
                                       PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                       VB_ALIGN);
    stVbConf.astCommPool[VB_POOL_MAIN_CHN].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[VB_POOL_MAIN_CHN].u32BlkCnt  = VB_BLK_MAIN_CNT;
    HI_LOG_I("Main VB: blkSize=%u cnt=%d", u32BlkSize, VB_BLK_MAIN_CNT);

    u32BlkSize = mpp_vb_calc_blk_size(pstConf->video.stSub.u32Width,
                                       pstConf->video.stSub.u32Height,
                                       PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                       VB_ALIGN);
    stVbConf.astCommPool[VB_POOL_SUB_CHN].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[VB_POOL_SUB_CHN].u32BlkCnt  = VB_BLK_SUB_CNT;
    HI_LOG_I("Sub VB: blkSize=%u cnt=%d", u32BlkSize, VB_BLK_SUB_CNT);

    stVbConf.astCommPool[VB_POOL_VENC].u32BlkSize = 512 * 1024;
    stVbConf.astCommPool[VB_POOL_VENC].u32BlkCnt  = VB_BLK_VENC_CNT;

    stVbConf.astCommPool[VB_POOL_VDA].u32BlkSize =
        mpp_vb_calc_blk_size(352, 288, PIXEL_FORMAT_YVU_SEMIPLANAR_420, VB_ALIGN);
    stVbConf.astCommPool[VB_POOL_VDA].u32BlkCnt  = VB_BLK_VDA_CNT;

    s32Ret = HI_MPI_VB_SetConf(&stVbConf);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("HI_MPI_VB_SetConf failed: 0x%x", s32Ret);
        return s32Ret;
    }

    s32Ret = HI_MPI_VB_Init();
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("HI_MPI_VB_Init failed: 0x%x", s32Ret);
        return s32Ret;
    }

    HI_LOG_I("VB pool initialized");
    return HI_SUCCESS;
#else
    HI_LOG_I("VB init (stub mode), main=%ux%u sub=%ux%u",
             pstConf->video.stMain.u32Width, pstConf->video.stMain.u32Height,
             pstConf->video.stSub.u32Width,  pstConf->video.stSub.u32Height);
    return HI_SUCCESS;
#endif
}

HI_VOID mpp_vb_deinit(void)
{
#ifdef HISI_SDK_AVAILABLE
    HI_S32 s32Ret = HI_MPI_VB_Exit();
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_W("HI_MPI_VB_Exit failed: 0x%x", s32Ret);
    }
#endif
    HI_LOG_I("VB pool de-initialized");
}
