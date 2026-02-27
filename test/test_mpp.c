#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "hi_type.h"
#include "hi_common.h"
#include "hi_conf.h"
#include "config_parser.h"
#include "mpp_vb.h"

static void test_vb_calc_blk_size(void)
{
    HI_U32 sz;

    sz = mpp_vb_calc_blk_size(1920, 1080, PIXEL_FORMAT_YVU_SEMIPLANAR_420, 64);
    printf("[TEST] VB blkSize 1920x1080 SP420 = %u bytes (~%u MB)\n",
           sz, sz / (1024 * 1024));
    assert(sz > 0);

    sz = mpp_vb_calc_blk_size(1280, 720, PIXEL_FORMAT_YVU_SEMIPLANAR_420, 64);
    printf("[TEST] VB blkSize 1280x720  SP420 = %u bytes\n", sz);
    assert(sz > 0);

    sz = mpp_vb_calc_blk_size(352, 288, PIXEL_FORMAT_YVU_SEMIPLANAR_420, 64);
    printf("[TEST] VB blkSize 352x288   SP420 = %u bytes\n", sz);
    assert(sz > 0);

    printf("[PASS] test_vb_calc_blk_size\n");
}

static void test_default_config(void)
{
    IPCAMERA_CONF_S stConf;

    ipcamera_conf_set_default(&stConf);

    assert(stConf.video.stMain.u32Width  == 1920);
    assert(stConf.video.stMain.u32Height == 1080);
    assert(stConf.video.stSub.u32Width   == 1280);
    assert(stConf.video.stSub.u32Height  == 720);
    assert(stConf.network.u16RtspPort    == 554);
    assert(stConf.record.u32SliceTime    == 10);
    assert(stConf.record.u32MaxDays      == 7);

    printf("[PASS] test_default_config\n");
}

static void test_config_parse_missing(void)
{
    IPCAMERA_CONF_S stConf;
    HI_S32 s32Ret;

    s32Ret = config_parse("/nonexistent/path.conf", &stConf);
    assert(s32Ret == HI_SUCCESS);
    assert(stConf.video.stMain.u32Width == 1920);

    printf("[PASS] test_config_parse_missing\n");
}

int main(void)
{
    printf("=== MPP Unit Tests ===\n");

    test_vb_calc_blk_size();
    test_default_config();
    test_config_parse_missing();

    printf("=== All Tests Passed ===\n");
    return 0;
}
