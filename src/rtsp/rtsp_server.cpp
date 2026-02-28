#include "rtsp_server.h"
#include "h264_stream_source.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <pthread.h>
#include <unistd.h>

#ifdef LIVE555_AVAILABLE
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#endif

#define RTSP_RECV_BUF_SIZE  (2 * 1024 * 1024)

struct StreamEntry {
    std::string         name;
    int                 vencChn;
    VENC_TYPE_E         codec;
    uint32_t            fps;
    H264StreamSource   *h264Src;
    H265StreamSource   *h265Src;
};

struct RtspCtx {
#ifdef LIVE555_AVAILABLE
    TaskScheduler      *scheduler;
    UsageEnvironment   *env;
    RTSPServer         *server;
#endif
    std::map<int, StreamEntry> streams;
    pthread_t           tid;
    bool                running;
    bool                initialized;
    pthread_mutex_t     mutex;
    uint16_t            port;
};

static RtspCtx g_rtspCtx;

#ifdef LIVE555_AVAILABLE
static void* rtsp_event_loop(void* pArg)
{
    RtspCtx *pCtx = (RtspCtx *)pArg;
    pCtx->env->taskScheduler().doEventLoop(&pCtx->running);
    return nullptr;
}
#endif

extern "C" HI_S32 rtsp_server_init(const NETWORK_CONF_S *pstNetConf)
{
    if (!pstNetConf) {
        return HI_FAILURE;
    }

    g_rtspCtx.streams.clear();
    g_rtspCtx.tid         = 0;
    g_rtspCtx.running     = false;
    g_rtspCtx.initialized = false;
    pthread_mutex_init(&g_rtspCtx.mutex, nullptr);
    g_rtspCtx.port        = pstNetConf->u16RtspPort ? pstNetConf->u16RtspPort : 554;

#ifdef LIVE555_AVAILABLE
    g_rtspCtx.scheduler = BasicTaskScheduler::createNew();
    g_rtspCtx.env       = BasicUsageEnvironment::createNew(*g_rtspCtx.scheduler);

    UserAuthenticationDatabase *authDB = nullptr;
    if (pstNetConf->bRtspAuth &&
        pstNetConf->szRtspUser[0] != '\0') {
        authDB = new UserAuthenticationDatabase;
        authDB->addUserRecord(pstNetConf->szRtspUser, pstNetConf->szRtspPass);
    }

    g_rtspCtx.server = RTSPServer::createNew(*g_rtspCtx.env,
                                              g_rtspCtx.port,
                                              authDB,
                                              65);
    if (!g_rtspCtx.server) {
        fprintf(stderr, "RTSPServer::createNew failed: port=%u\n", g_rtspCtx.port);
        g_rtspCtx.env->reclaim();
        g_rtspCtx.env = nullptr;
        delete g_rtspCtx.scheduler;
        g_rtspCtx.scheduler = nullptr;
        return HI_FAILURE;
    }
#endif

    g_rtspCtx.initialized = true;
    fprintf(stdout, "RTSP server initialized on port %u\n", g_rtspCtx.port);
    return HI_SUCCESS;
}

extern "C" HI_VOID rtsp_server_deinit(void)
{
    if (!g_rtspCtx.initialized) {
        return;
    }

    rtsp_server_stop();

#ifdef LIVE555_AVAILABLE
    if (g_rtspCtx.server) {
        Medium::close(g_rtspCtx.server);
        g_rtspCtx.server = nullptr;
    }
    if (g_rtspCtx.env) {
        g_rtspCtx.env->reclaim();
        g_rtspCtx.env = nullptr;
    }
    if (g_rtspCtx.scheduler) {
        delete g_rtspCtx.scheduler;
        g_rtspCtx.scheduler = nullptr;
    }
#endif

    pthread_mutex_destroy(&g_rtspCtx.mutex);
    g_rtspCtx.initialized = false;
}

extern "C" HI_S32 rtsp_server_start(void)
{
    if (!g_rtspCtx.initialized) {
        return HI_FAILURE;
    }

    g_rtspCtx.running = true;

#ifdef LIVE555_AVAILABLE
    if (pthread_create(&g_rtspCtx.tid, nullptr, rtsp_event_loop, &g_rtspCtx) != 0) {
        fprintf(stderr, "Failed to create RTSP event loop thread\n");
        g_rtspCtx.running = false;
        return HI_FAILURE;
    }
#endif

    fprintf(stdout, "RTSP server started\n");
    return HI_SUCCESS;
}

extern "C" HI_VOID rtsp_server_stop(void)
{
    if (!g_rtspCtx.running) {
        return;
    }

    g_rtspCtx.running = false;

#ifdef LIVE555_AVAILABLE
    if (g_rtspCtx.tid != 0) {
        pthread_join(g_rtspCtx.tid, nullptr);
        g_rtspCtx.tid = 0;
    }
#endif

    fprintf(stdout, "RTSP server stopped\n");
}

extern "C" HI_S32 rtsp_server_add_stream(const char *pszName, HI_S32 s32VencChn,
                                          VENC_TYPE_E enCodec, HI_U32 u32Fps)
{
    if (!pszName || !g_rtspCtx.initialized) {
        return HI_FAILURE;
    }

    pthread_mutex_lock(&g_rtspCtx.mutex);

    StreamEntry entry;
    entry.name     = pszName;
    entry.vencChn  = s32VencChn;
    entry.codec    = enCodec;
    entry.fps      = u32Fps;
    entry.h264Src  = nullptr;
    entry.h265Src  = nullptr;

#ifdef LIVE555_AVAILABLE
    ServerMediaSession *sms = ServerMediaSession::createNew(
        *g_rtspCtx.env, pszName, pszName, "IPCamera Stream");

    if (enCodec == VENC_TYPE_H265) {
        entry.h265Src = H265StreamSource::createNew(*g_rtspCtx.env, s32VencChn);
        H265VideoRTPSink *sink = H265VideoRTPSink::createNew(*g_rtspCtx.env, nullptr, 96);
        PassiveServerMediaSubsession *sub =
            PassiveServerMediaSubsession::createNew(*sink, nullptr);
        sms->addSubsession(sub);
    } else {
        entry.h264Src = H264StreamSource::createNew(*g_rtspCtx.env, s32VencChn);
        H264VideoRTPSink *sink = H264VideoRTPSink::createNew(*g_rtspCtx.env, nullptr, 96);
        PassiveServerMediaSubsession *sub =
            PassiveServerMediaSubsession::createNew(*sink, nullptr);
        sms->addSubsession(sub);
    }

    g_rtspCtx.server->addServerMediaSession(sms);

    char *url = g_rtspCtx.server->rtspURL(sms);
    fprintf(stdout, "Stream added: %s -> %s\n", pszName, url);
    delete[] url;
#endif

    g_rtspCtx.streams[s32VencChn] = entry;
    pthread_mutex_unlock(&g_rtspCtx.mutex);

    fprintf(stdout, "Stream registered: name=%s chn=%d codec=%d fps=%u\n",
            pszName, s32VencChn, (int)enCodec, u32Fps);
    return HI_SUCCESS;
}

extern "C" HI_VOID rtsp_server_remove_stream(const char *pszName)
{
    if (!pszName) {
        return;
    }

    pthread_mutex_lock(&g_rtspCtx.mutex);

    for (auto it = g_rtspCtx.streams.begin(); it != g_rtspCtx.streams.end(); ) {
        if (it->second.name == pszName) {
#ifdef LIVE555_AVAILABLE
            if (g_rtspCtx.server) {
                g_rtspCtx.server->removeServerMediaSession(pszName);
            }
#endif
            it = g_rtspCtx.streams.erase(it);
        } else {
            ++it;
        }
    }

    pthread_mutex_unlock(&g_rtspCtx.mutex);
}

extern "C" HI_S32 rtsp_server_feed_frame(HI_S32 s32VencChn, const HI_U8 *pData,
                                          HI_U32 u32Len, HI_U64 u64Pts,
                                          HI_BOOL bKeyFrame)
{
    if (!pData || u32Len == 0) {
        return HI_FAILURE;
    }

    pthread_mutex_lock(&g_rtspCtx.mutex);

    auto it = g_rtspCtx.streams.find(s32VencChn);
    if (it != g_rtspCtx.streams.end()) {
        StreamEntry& entry = it->second;
        if (entry.h264Src) {
            entry.h264Src->feedFrame(pData, u32Len, u64Pts, bKeyFrame != HI_FALSE);
        } else if (entry.h265Src) {
            entry.h265Src->feedFrame(pData, u32Len, u64Pts, bKeyFrame != HI_FALSE);
        }
    }

    pthread_mutex_unlock(&g_rtspCtx.mutex);
    return HI_SUCCESS;
}

extern "C" HI_S32 rtsp_server_get_client_count(const char *pszName)
{
    int count = 0;

#ifdef LIVE555_AVAILABLE
    if (pszName && g_rtspCtx.server) {
        ServerMediaSession *sms =
            g_rtspCtx.server->lookupServerMediaSession(pszName);
        if (sms) {
            count = (int)sms->referenceCount();
        }
    }
#else
    (void)pszName;
#endif

    return count;
}

extern "C" HI_S32 rtsp_server_request_idr(HI_S32 s32VencChn)
{
    (void)s32VencChn;
    return HI_SUCCESS;
}
