#include "mpp_bind.h"
#include "hi_log.h"
#include "hi_common.h"

#ifdef HISI_SDK_AVAILABLE
#include "hi_mpi_sys.h"

typedef struct {
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDstChn;
} BIND_PAIR_S;

static HI_S32 do_bind(const MPP_CHN_S *pstSrc, const MPP_CHN_S *pstDst)
{
    HI_S32 s32Ret = HI_MPI_SYS_Bind(pstSrc, pstDst);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("Bind [%d/%d/%d]->[%d/%d/%d] failed: 0x%x",
                 pstSrc->enModId, pstSrc->s32DevId, pstSrc->s32ChnId,
                 pstDst->enModId, pstDst->s32DevId, pstDst->s32ChnId,
                 s32Ret);
    }
    return s32Ret;
}

static HI_VOID do_unbind(const MPP_CHN_S *pstSrc, const MPP_CHN_S *pstDst)
{
    HI_S32 s32Ret = HI_MPI_SYS_UnBind(pstSrc, pstDst);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_W("UnBind [%d/%d/%d]->[%d/%d/%d] failed: 0x%x",
                 pstSrc->enModId, pstSrc->s32DevId, pstSrc->s32ChnId,
                 pstDst->enModId, pstDst->s32DevId, pstDst->s32ChnId,
                 s32Ret);
    }
}
#endif

HI_S32 mpp_bind_modules(void)
{
#ifdef HISI_SDK_AVAILABLE
    MPP_CHN_S stViChn, stVpssGrp, stVpssChnMain, stVpssChnSub, stVpssChnVda;
    MPP_CHN_S stVencChnMain, stVencChnSub, stVdaChn;
    HI_S32    s32Ret;

    stViChn.enModId    = HI_ID_VIU;
    stViChn.s32DevId   = VI_PIPE_MAIN;
    stViChn.s32ChnId   = VI_CHN_MAIN;

    stVpssGrp.enModId  = HI_ID_VPSS;
    stVpssGrp.s32DevId = VPSS_GRP_MAIN;
    stVpssGrp.s32ChnId = 0;

    s32Ret = do_bind(&stViChn, &stVpssGrp);
    if (HI_SUCCESS != s32Ret) {
        return s32Ret;
    }

    stVpssChnMain.enModId  = HI_ID_VPSS;
    stVpssChnMain.s32DevId = VPSS_GRP_MAIN;
    stVpssChnMain.s32ChnId = VPSS_CHN_MAIN;
    stVencChnMain.enModId  = HI_ID_VENC;
    stVencChnMain.s32DevId = 0;
    stVencChnMain.s32ChnId = VENC_CHN_MAIN;
    s32Ret = do_bind(&stVpssChnMain, &stVencChnMain);
    if (HI_SUCCESS != s32Ret) {
        do_unbind(&stViChn, &stVpssGrp);
        return s32Ret;
    }

    stVpssChnSub.enModId   = HI_ID_VPSS;
    stVpssChnSub.s32DevId  = VPSS_GRP_MAIN;
    stVpssChnSub.s32ChnId  = VPSS_CHN_SUB;
    stVencChnSub.enModId   = HI_ID_VENC;
    stVencChnSub.s32DevId  = 0;
    stVencChnSub.s32ChnId  = VENC_CHN_SUB;
    s32Ret = do_bind(&stVpssChnSub, &stVencChnSub);
    if (HI_SUCCESS != s32Ret) {
        do_unbind(&stVpssChnMain, &stVencChnMain);
        do_unbind(&stViChn, &stVpssGrp);
        return s32Ret;
    }

    stVpssChnVda.enModId   = HI_ID_VPSS;
    stVpssChnVda.s32DevId  = VPSS_GRP_MAIN;
    stVpssChnVda.s32ChnId  = VPSS_CHN_VDA;
    stVdaChn.enModId       = HI_ID_VDA;
    stVdaChn.s32DevId      = 0;
    stVdaChn.s32ChnId      = VDA_CHN_MAIN;
    do_bind(&stVpssChnVda, &stVdaChn);

    HI_LOG_I("Module binding: VI->VPSS->VENC(main/sub), VPSS->VDA");
    return HI_SUCCESS;
#else
    HI_LOG_I("Module bind (stub mode)");
    return HI_SUCCESS;
#endif
}

HI_VOID mpp_unbind_modules(void)
{
#ifdef HISI_SDK_AVAILABLE
    MPP_CHN_S stSrc, stDst;

    stSrc.enModId  = HI_ID_VPSS;
    stSrc.s32DevId = VPSS_GRP_MAIN;
    stSrc.s32ChnId = VPSS_CHN_VDA;
    stDst.enModId  = HI_ID_VDA;
    stDst.s32DevId = 0;
    stDst.s32ChnId = VDA_CHN_MAIN;
    do_unbind(&stSrc, &stDst);

    stSrc.s32ChnId = VPSS_CHN_SUB;
    stDst.enModId  = HI_ID_VENC;
    stDst.s32ChnId = VENC_CHN_SUB;
    do_unbind(&stSrc, &stDst);

    stSrc.s32ChnId = VPSS_CHN_MAIN;
    stDst.s32ChnId = VENC_CHN_MAIN;
    do_unbind(&stSrc, &stDst);

    stSrc.enModId  = HI_ID_VIU;
    stSrc.s32DevId = VI_PIPE_MAIN;
    stSrc.s32ChnId = VI_CHN_MAIN;
    stDst.enModId  = HI_ID_VPSS;
    stDst.s32DevId = VPSS_GRP_MAIN;
    stDst.s32ChnId = 0;
    do_unbind(&stSrc, &stDst);
#endif
    HI_LOG_I("Module unbound");
}
