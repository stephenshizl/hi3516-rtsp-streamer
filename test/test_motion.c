#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "hi_type.h"
#include "hi_conf.h"
#include "hi_log.h"
#include "motion_detect.h"

static HI_BOOL g_bAlarmFired = HI_FALSE;
static HI_BOOL g_bLastAlarm  = HI_FALSE;

static void test_alarm_cb(HI_U32 u32Region, HI_BOOL bAlarm, void *pPriv)
{
    printf("[CB] Motion region=%u alarm=%d\n", u32Region, bAlarm);
    g_bAlarmFired = HI_TRUE;
    g_bLastAlarm  = bAlarm;
}

static void test_motion_region(void)
{
    MOTION_CONF_S        stConf;
    MOTION_REGION_CONF_S stRegion;
    MOTION_REGION_CONF_S stReadBack;
    HI_S32 s32Ret;

    memset(&stConf, 0, sizeof(stConf));
    stConf.bEnable       = HI_TRUE;
    stConf.u32RegionCnt  = 1;
    stConf.astRegions[0].s32X    = 0;
    stConf.astRegions[0].s32Y    = 0;
    stConf.astRegions[0].u32Width  = 1920;
    stConf.astRegions[0].u32Height = 1080;
    stConf.astRegions[0].bEnable   = HI_TRUE;
    stConf.astRegions[0].u32Sensitivity = 5;
    stConf.astRegions[0].u32Threshold   = 40;

    s32Ret = motion_detect_init(&stConf);
    assert(s32Ret == HI_SUCCESS);

    motion_detect_register_callback(test_alarm_cb, HI_NULL);

    memset(&stRegion, 0, sizeof(stRegion));
    stRegion.s32X    = 100;
    stRegion.s32Y    = 100;
    stRegion.u32Width  = 640;
    stRegion.u32Height = 480;
    stRegion.bEnable   = HI_TRUE;
    stRegion.u32Sensitivity = 7;
    stRegion.u32Threshold   = 25;

    s32Ret = motion_detect_set_region(0, &stRegion);
    assert(s32Ret == HI_SUCCESS);

    s32Ret = motion_detect_get_region(0, &stReadBack);
    assert(s32Ret == HI_SUCCESS);
    assert(stReadBack.s32X     == 100);
    assert(stReadBack.u32Width == 640);
    assert(stReadBack.u32Sensitivity == 7);

    s32Ret = motion_detect_set_region(MAX_MOTION_REGIONS, &stRegion);
    assert(s32Ret == HI_FAILURE);

    assert(motion_detect_is_alarming() == HI_FALSE);

    motion_detect_deinit();
    printf("[PASS] test_motion_region\n");
}

int main(void)
{
    LOG_CONF_S stLogConf;

    memset(&stLogConf, 0, sizeof(stLogConf));
    stLogConf.enLevel  = LOG_LEVEL_DEBUG;
    stLogConf.enOutput = LOG_OUTPUT_STDOUT;
    hi_log_init(&stLogConf);

    printf("=== Motion Detect Tests ===\n");
    test_motion_region();
    printf("=== All Tests Passed ===\n");

    hi_log_deinit();
    return 0;
}
