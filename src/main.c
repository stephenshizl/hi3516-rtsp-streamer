#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#include "hi_type.h"
#include "hi_common.h"
#include "hi_log.h"
#include "hi_conf.h"
#include "config_parser.h"
#include "mpp_sys.h"
#include "mpp_vb.h"
#include "mpp_vi.h"
#include "mpp_vpss.h"
#include "mpp_venc.h"
#include "mpp_vda.h"
#include "mpp_bind.h"
#include "rtsp_server.h"
#include "record_manager.h"
#include "motion_detect.h"
#include "sd_card_manager.h"
#include "circular_storage.h"
#include "hi_utils.h"

#define MAIN_LOOP_PERIOD_MS   1000
#define SD_CHECK_PERIOD_S     10
#define CS_CHECK_PERIOD_S     30

static volatile HI_BOOL   g_bRunning = HI_TRUE;
static IPCAMERA_CONF_S    g_stConf;
static HI_U32             g_u32SdCheckCnt = 0;
static HI_U32             g_u32CsCheckCnt = 0;

static void signal_handler(int sig)
{
    printf("\nReceived signal %d, shutting down...\n", sig);
    g_bRunning = HI_FALSE;
}

static void on_venc_frame(HI_S32 s32Chn, const VENC_FRAME_S *pstFrame,
                           void *pPrivData)
{
    if (HI_NULL == pstFrame || HI_NULL == pstFrame->pData) {
        return;
    }

    rtsp_server_feed_frame(s32Chn, pstFrame->pData, pstFrame->u32Len,
                           pstFrame->u64Pts, pstFrame->bKeyFrame);

    record_manager_feed_frame(s32Chn, pstFrame->pData, pstFrame->u32Len,
                              pstFrame->u64Pts, pstFrame->bKeyFrame);
}

static void on_motion_alarm(HI_U32 u32Region, HI_BOOL bAlarm, void *pPriv)
{
    HI_LOG_I("Motion [region=%u]: %s", u32Region, bAlarm ? "ALARM" : "CLEAR");
    record_manager_on_motion(bAlarm);
}

static HI_S32 init_log(void)
{
    LOG_CONF_S stLogConf;

    memset(&stLogConf, 0, sizeof(stLogConf));
    stLogConf.enLevel  = LOG_LEVEL_INFO;
    stLogConf.enOutput = LOG_OUTPUT_STDOUT;

    return hi_log_init(&stLogConf);
}

static HI_S32 init_mpp(void)
{
    HI_S32 s32Ret;
    VI_CONF_S stViConf;

    s32Ret = mpp_sys_init();
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("mpp_sys_init failed: 0x%x", s32Ret);
        return s32Ret;
    }

    s32Ret = mpp_vb_init(&g_stConf);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("mpp_vb_init failed: 0x%x", s32Ret);
        mpp_sys_deinit();
        return s32Ret;
    }

    memset(&stViConf, 0, sizeof(stViConf));
    stViConf.u32Width    = g_stConf.video.stMain.u32Width;
    stViConf.u32Height   = g_stConf.video.stMain.u32Height;
    stViConf.u32FrameRate = g_stConf.video.u32SensorFps;
    stViConf.enViMode    = VI_MODE_MIPI;
    stViConf.enSensor    = SENSOR_IMX335;
    stViConf.enPixFmt    = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    stViConf.bWdrMode    = HI_FALSE;

    s32Ret = mpp_vi_init(&stViConf);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("mpp_vi_init failed: 0x%x", s32Ret);
        mpp_vb_deinit();
        mpp_sys_deinit();
        return s32Ret;
    }

    s32Ret = mpp_vpss_init(&g_stConf.video);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("mpp_vpss_init failed: 0x%x", s32Ret);
        mpp_vi_deinit();
        mpp_vb_deinit();
        mpp_sys_deinit();
        return s32Ret;
    }

    s32Ret = mpp_venc_init(&g_stConf.video.stMain, &g_stConf.video.stSub);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("mpp_venc_init failed: 0x%x", s32Ret);
        mpp_vpss_deinit();
        mpp_vi_deinit();
        mpp_vb_deinit();
        mpp_sys_deinit();
        return s32Ret;
    }

    if (g_stConf.motion.bEnable) {
        s32Ret = mpp_vda_init(&g_stConf.motion);
        if (HI_SUCCESS != s32Ret) {
            HI_LOG_W("mpp_vda_init failed (motion detect disabled): 0x%x", s32Ret);
        }
    }

    s32Ret = mpp_bind_modules();
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("mpp_bind_modules failed: 0x%x", s32Ret);
        mpp_vda_deinit();
        mpp_venc_deinit();
        mpp_vpss_deinit();
        mpp_vi_deinit();
        mpp_vb_deinit();
        mpp_sys_deinit();
        return s32Ret;
    }

    HI_LOG_I("MPP initialized");
    return HI_SUCCESS;
}

static void deinit_mpp(void)
{
    mpp_unbind_modules();
    mpp_vda_deinit();
    mpp_venc_deinit();
    mpp_vpss_deinit();
    mpp_vi_deinit();
    mpp_vb_deinit();
    mpp_sys_deinit();
}

static HI_S32 init_rtsp(void)
{
    HI_S32 s32Ret;
    const char *pszCodec;

    s32Ret = rtsp_server_init(&g_stConf.network);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("rtsp_server_init failed: 0x%x", s32Ret);
        return s32Ret;
    }

    s32Ret = rtsp_server_add_stream("stream0", VENC_CHN_MAIN,
                                     g_stConf.video.stMain.enCodec,
                                     g_stConf.video.stMain.u32FrameRate);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("add stream0 failed");
        rtsp_server_deinit();
        return s32Ret;
    }

    s32Ret = rtsp_server_add_stream("stream1", VENC_CHN_SUB,
                                     g_stConf.video.stSub.enCodec,
                                     g_stConf.video.stSub.u32FrameRate);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("add stream1 failed");
        rtsp_server_deinit();
        return s32Ret;
    }

    mpp_venc_register_callback(VENC_CHN_MAIN, on_venc_frame, HI_NULL);
    mpp_venc_register_callback(VENC_CHN_SUB,  on_venc_frame, HI_NULL);

    s32Ret = rtsp_server_start();
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("rtsp_server_start failed");
        rtsp_server_deinit();
        return s32Ret;
    }

    pszCodec = (g_stConf.video.stMain.enCodec == VENC_TYPE_H265) ? "H.265" : "H.264";
    HI_LOG_I("RTSP server started: rtsp://<ip>:%u/stream0 (%s %ux%u@%ukbps)",
             g_stConf.network.u16RtspPort, pszCodec,
             g_stConf.video.stMain.u32Width, g_stConf.video.stMain.u32Height,
             g_stConf.video.stMain.u32BitRate);
    HI_LOG_I("RTSP sub stream:    rtsp://<ip>:%u/stream1 (%s %ux%u@%ukbps)",
             g_stConf.network.u16RtspPort, pszCodec,
             g_stConf.video.stSub.u32Width, g_stConf.video.stSub.u32Height,
             g_stConf.video.stSub.u32BitRate);

    return HI_SUCCESS;
}

static HI_S32 init_record(void)
{
    HI_S32 s32Ret;

    if (!g_stConf.record.bEnable) {
        HI_LOG_I("Recording disabled in config");
        return HI_SUCCESS;
    }

    s32Ret = sdcard_init(g_stConf.record.szMountPoint);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_W("sdcard_init failed, record may not work");
    }

    s32Ret = record_manager_init(&g_stConf.record);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("record_manager_init failed: 0x%x", s32Ret);
        return s32Ret;
    }

    s32Ret = record_manager_start();
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("record_manager_start failed");
        return s32Ret;
    }

    return HI_SUCCESS;
}

static HI_S32 init_motion(void)
{
    HI_S32 s32Ret;

    if (!g_stConf.motion.bEnable) {
        HI_LOG_I("Motion detection disabled in config");
        return HI_SUCCESS;
    }

    s32Ret = motion_detect_init(&g_stConf.motion);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("motion_detect_init failed: 0x%x", s32Ret);
        return s32Ret;
    }

    motion_detect_register_callback(on_motion_alarm, HI_NULL);

    s32Ret = motion_detect_start();
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("motion_detect_start failed");
        return s32Ret;
    }

    return HI_SUCCESS;
}

static void periodic_maintenance(void)
{
    g_u32SdCheckCnt++;
    if (g_u32SdCheckCnt >= SD_CHECK_PERIOD_S) {
        g_u32SdCheckCnt = 0;
        sdcard_check();
    }

    g_u32CsCheckCnt++;
    if (g_u32CsCheckCnt >= CS_CHECK_PERIOD_S) {
        g_u32CsCheckCnt = 0;
        circular_storage_check();
    }
}

static void print_usage(const char *pszProg)
{
    printf("Usage: %s [OPTIONS]\n\n"
           "Options:\n"
           "  -c <file>   Config file path (default: %s)\n"
           "  -l <level>  Log level (0=debug,1=info,2=warn,3=error)\n"
           "  -h          Show this help\n\n"
           "RTSP URLs:\n"
           "  Main stream: rtsp://<ip>:554/stream0\n"
           "  Sub stream:  rtsp://<ip>:554/stream1\n",
           pszProg, DEFAULT_CONF_FILE);
}

int main(int argc, char *argv[])
{
    HI_S32       s32Ret = HI_SUCCESS;
    const char  *pszConfFile  = DEFAULT_CONF_FILE;
    HI_S32       s32LogLevel  = LOG_LEVEL_INFO;
    int          opt;

    while ((opt = getopt(argc, argv, "c:l:h")) != -1) {
        switch (opt) {
        case 'c':
            pszConfFile = optarg;
            break;
        case 'l':
            s32LogLevel = atoi(optarg);
            if (s32LogLevel < 0 || s32LogLevel >= LOG_LEVEL_BUTT) {
                s32LogLevel = LOG_LEVEL_INFO;
            }
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }

    s32Ret = init_log();
    if (HI_SUCCESS != s32Ret) {
        fprintf(stderr, "init_log failed\n");
        return 1;
    }
    hi_log_set_level((LOG_LEVEL_E)s32LogLevel);

    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);

    HI_LOG_I("====================================");
    HI_LOG_I(" Hi3516CV300 IPCamera RTSP Streamer");
    HI_LOG_I("====================================");

    s32Ret = config_parse(pszConfFile, &g_stConf);
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_E("config_parse failed");
        goto EXIT_LOG;
    }

    s32Ret = init_mpp();
    if (HI_SUCCESS != s32Ret) {
        goto EXIT_LOG;
    }

    s32Ret = init_rtsp();
    if (HI_SUCCESS != s32Ret) {
        goto EXIT_MPP;
    }

    s32Ret = init_motion();
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_W("Motion detect init failed, continuing without");
    }

    s32Ret = init_record();
    if (HI_SUCCESS != s32Ret) {
        HI_LOG_W("Record init failed, continuing without");
    }

    HI_LOG_I("System started successfully");

    while (g_bRunning) {
        hi_sleep_ms(MAIN_LOOP_PERIOD_MS);
        periodic_maintenance();
    }

    HI_LOG_I("Shutting down...");

    if (g_stConf.record.bEnable) {
        record_manager_stop();
        record_manager_deinit();
        sdcard_deinit();
    }

    if (g_stConf.motion.bEnable) {
        motion_detect_stop();
        motion_detect_deinit();
    }

    mpp_venc_unregister_callback(VENC_CHN_MAIN);
    mpp_venc_unregister_callback(VENC_CHN_SUB);

    rtsp_server_stop();
    rtsp_server_deinit();

EXIT_MPP:
    deinit_mpp();

EXIT_LOG:
    hi_log_deinit();

    printf("Bye.\n");
    return (s32Ret == HI_SUCCESS) ? 0 : 1;
}
