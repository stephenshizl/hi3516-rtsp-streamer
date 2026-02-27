#include "mpp_sys.h"
#include "hi_log.h"
#include <string.h>

#ifdef HISI_SDK_AVAILABLE
#include "hi_mpi_sys.h"
#include "hi_mpi_vb.h"
#endif

#define MPP_SYS_MMZ_SIZE_MB   256
#define MPP_SYS_ALIGN_WIDTH   64

HI_S32 mpp_sys_init(void)
{
#ifdef HISI_SDK_AVAILABLE
    MPP_SYS_CONF_S stSysConf;
    HI_S32 s32Ret;

    memset(&stSysConf, 0, sizeof(stSysConf));
    stSysConf.u32AlignWidth = MPP_SYS_ALIGN_WIDTH;

    s32Ret = HI_MPI_SYS_SetConf(&stSysConf);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("HI_MPI_SYS_SetConf failed: 0x%x", s32Ret);
        return s32Ret;
    }

    s32Ret = HI_MPI_SYS_Init();
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("HI_MPI_SYS_Init failed: 0x%x", s32Ret);
        return s32Ret;
    }

    HI_LOG_I("MPP system initialized, MMZ=%dMB", MPP_SYS_MMZ_SIZE_MB);
    return HI_SUCCESS;
#else
    HI_LOG_I("MPP system init (stub mode)");
    return HI_SUCCESS;
#endif
}

HI_VOID mpp_sys_deinit(void)
{
#ifdef HISI_SDK_AVAILABLE
    HI_S32 s32Ret = HI_MPI_SYS_Exit();
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_W("HI_MPI_SYS_Exit failed: 0x%x", s32Ret);
    }
#endif
    HI_LOG_I("MPP system de-initialized");
}
