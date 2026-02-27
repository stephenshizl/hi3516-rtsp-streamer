#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "hi_type.h"
#include "hi_conf.h"
#include "hi_log.h"
#include "hi_utils.h"
#include "rtsp_server.h"

static NETWORK_CONF_S make_default_net_conf(void)
{
    NETWORK_CONF_S stConf;
    memset(&stConf, 0, sizeof(stConf));
    strncpy(stConf.szIp, "0.0.0.0", sizeof(stConf.szIp) - 1);
    stConf.u16RtspPort = 8554;
    stConf.bRtspAuth   = HI_FALSE;
    return stConf;
}

static void test_rtsp_init_deinit(void)
{
    NETWORK_CONF_S stConf = make_default_net_conf();
    HI_S32 s32Ret;

    s32Ret = rtsp_server_init(&stConf);
    assert(s32Ret == HI_SUCCESS);

    rtsp_server_deinit();
    printf("[PASS] test_rtsp_init_deinit\n");
}

static void test_rtsp_add_stream(void)
{
    NETWORK_CONF_S stConf = make_default_net_conf();
    HI_S32 s32Ret;

    s32Ret = rtsp_server_init(&stConf);
    assert(s32Ret == HI_SUCCESS);

    s32Ret = rtsp_server_add_stream("stream0", 0, VENC_TYPE_H264, 25);
    assert(s32Ret == HI_SUCCESS);

    s32Ret = rtsp_server_add_stream("stream1", 1, VENC_TYPE_H264, 25);
    assert(s32Ret == HI_SUCCESS);

    s32Ret = rtsp_server_start();
    assert(s32Ret == HI_SUCCESS);

    hi_sleep_ms(100);

    rtsp_server_stop();

    rtsp_server_remove_stream("stream0");
    rtsp_server_remove_stream("stream1");
    rtsp_server_deinit();

    printf("[PASS] test_rtsp_add_stream\n");
}

static void test_rtsp_feed_frame(void)
{
    NETWORK_CONF_S stConf = make_default_net_conf();
    HI_S32 s32Ret;

    static const HI_U8 s_h264IdrNalHeader[] = {
        0x00, 0x00, 0x00, 0x01, 0x65
    };
    static const HI_U8 s_h264SpsPps[] = {
        0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xC0, 0x1E,
        0x00, 0x00, 0x00, 0x01, 0x68, 0xCE, 0x38, 0x80
    };

    s32Ret = rtsp_server_init(&stConf);
    assert(s32Ret == HI_SUCCESS);

    s32Ret = rtsp_server_add_stream("test", 0, VENC_TYPE_H264, 25);
    assert(s32Ret == HI_SUCCESS);

    s32Ret = rtsp_server_feed_frame(0, s_h264SpsPps, sizeof(s_h264SpsPps),
                                     0, HI_TRUE);
    assert(s32Ret == HI_SUCCESS);

    s32Ret = rtsp_server_feed_frame(0, s_h264IdrNalHeader,
                                     sizeof(s_h264IdrNalHeader),
                                     40000, HI_TRUE);
    assert(s32Ret == HI_SUCCESS);

    s32Ret = rtsp_server_feed_frame(99, s_h264IdrNalHeader,
                                     sizeof(s_h264IdrNalHeader),
                                     0, HI_FALSE);
    assert(s32Ret == HI_SUCCESS);

    rtsp_server_deinit();
    printf("[PASS] test_rtsp_feed_frame\n");
}

static void test_rtsp_null_args(void)
{
    HI_S32 s32Ret;

    s32Ret = rtsp_server_init(HI_NULL);
    assert(s32Ret == HI_FAILURE);

    s32Ret = rtsp_server_feed_frame(0, HI_NULL, 0, 0, HI_FALSE);
    assert(s32Ret == HI_FAILURE);

    printf("[PASS] test_rtsp_null_args\n");
}

int main(void)
{
    LOG_CONF_S stLogConf;

    memset(&stLogConf, 0, sizeof(stLogConf));
    stLogConf.enLevel  = LOG_LEVEL_DEBUG;
    stLogConf.enOutput = LOG_OUTPUT_STDOUT;
    hi_log_init(&stLogConf);

    printf("=== RTSP Server Tests ===\n");

    test_rtsp_null_args();
    test_rtsp_init_deinit();
    test_rtsp_add_stream();
    test_rtsp_feed_frame();

    printf("=== All Tests Passed ===\n");

    hi_log_deinit();
    return 0;
}
