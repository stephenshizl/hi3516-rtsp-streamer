#include "h264_stream_source.h"
#include <cstring>
#include <cstdio>
#include <unistd.h>

static const uint8_t NAL_START_CODE[] = {0x00, 0x00, 0x00, 0x01};

H264StreamSource* H264StreamSource::createNew(UsageEnvironment& env, int vencChn)
{
    return new H264StreamSource(env, vencChn);
}

H264StreamSource::H264StreamSource(UsageEnvironment& env, int vencChn)
    : FramedSource(env)
    , m_vencChn(vencChn)
    , m_hasSps(false)
    , m_hasPps(false)
    , m_active(false)
{
    pthread_mutex_init(&m_mutex, nullptr);
    pthread_cond_init(&m_cond, nullptr);
}

H264StreamSource::~H264StreamSource()
{
    pthread_mutex_lock(&m_mutex);
    m_active = false;
    pthread_cond_broadcast(&m_cond);
    pthread_mutex_unlock(&m_mutex);

    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);
}

void H264StreamSource::feedFrame(const uint8_t* pData, uint32_t u32Len,
                                  uint64_t u64Pts, bool bKeyFrame)
{
    if (!pData || u32Len == 0) {
        return;
    }

    parseSpsPps(pData, u32Len);

    pthread_mutex_lock(&m_mutex);

    if (m_frameQueue.size() < STREAM_FRAME_QUEUE_MAX) {
        StreamFrame frame;
        frame.data.assign(pData, pData + u32Len);
        frame.pts      = u64Pts;
        frame.keyFrame = bKeyFrame;
        m_frameQueue.push(std::move(frame));
        pthread_cond_signal(&m_cond);
    }

    pthread_mutex_unlock(&m_mutex);
}

void H264StreamSource::parseSpsPps(const uint8_t* pData, uint32_t u32Len)
{
    if (m_hasSps && m_hasPps) {
        return;
    }

    uint32_t i = 0;
    while (i + 4 < u32Len) {
        if (pData[i] == 0x00 && pData[i+1] == 0x00 &&
            pData[i+2] == 0x00 && pData[i+3] == 0x01) {
            uint32_t naluStart = i + 4;
            uint8_t  naluType  = pData[naluStart] & 0x1F;

            uint32_t naluEnd = u32Len;
            for (uint32_t j = naluStart + 1; j + 3 < u32Len; j++) {
                if (pData[j] == 0x00 && pData[j+1] == 0x00 &&
                    pData[j+2] == 0x00 && pData[j+3] == 0x01) {
                    naluEnd = j;
                    break;
                }
            }

            if (naluType == 7 && !m_hasSps) {
                m_sps.assign(pData + naluStart, pData + naluEnd);
                m_hasSps = true;
            } else if (naluType == 8 && !m_hasPps) {
                m_pps.assign(pData + naluStart, pData + naluEnd);
                m_hasPps = true;
            }

            i = naluEnd;
        } else {
            i++;
        }
    }
}

bool H264StreamSource::findNalu(const uint8_t* pData, uint32_t u32Len,
                                 uint32_t& startOff, uint32_t& naluLen)
{
    for (uint32_t i = 0; i + 3 < u32Len; i++) {
        if (pData[i] == 0x00 && pData[i+1] == 0x00 &&
            pData[i+2] == 0x00 && pData[i+3] == 0x01) {
            startOff = i + 4;
            naluLen  = u32Len - startOff;
            return true;
        }
    }
    return false;
}

void H264StreamSource::doGetNextFrame()
{
    m_active = true;

    pthread_mutex_lock(&m_mutex);

    while (m_frameQueue.empty() && m_active) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += 40 * 1000000;
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000L;
        }
        pthread_cond_timedwait(&m_cond, &m_mutex, &ts);
    }

    if (!m_active || m_frameQueue.empty()) {
        pthread_mutex_unlock(&m_mutex);
        nextTask() = envir().taskScheduler().scheduleDelayedTask(
            0, (TaskFunc*)afterGetting, this);
        return;
    }

    StreamFrame frame = std::move(m_frameQueue.front());
    m_frameQueue.pop();
    pthread_mutex_unlock(&m_mutex);

    if (frame.data.size() > fMaxSize) {
        fFrameSize     = fMaxSize;
        fNumTruncatedBytes = frame.data.size() - fMaxSize;
    } else {
        fFrameSize = frame.data.size();
        fNumTruncatedBytes = 0;
    }

    memcpy(fTo, frame.data.data(), fFrameSize);

    fPresentationTime.tv_sec  = (long)(frame.pts / 1000000);
    fPresentationTime.tv_usec = (long)(frame.pts % 1000000);
    fDurationInMicroseconds   = 0;

    afterGetting(this);
}

void H264StreamSource::doStopGettingFrames()
{
    m_active = false;
    pthread_mutex_lock(&m_mutex);
    pthread_cond_broadcast(&m_cond);
    pthread_mutex_unlock(&m_mutex);
}

const uint8_t* H264StreamSource::getSps(size_t& len) const
{
    if (!m_hasSps) { len = 0; return nullptr; }
    len = m_sps.size();
    return m_sps.data();
}

const uint8_t* H264StreamSource::getPps(size_t& len) const
{
    if (!m_hasPps) { len = 0; return nullptr; }
    len = m_pps.size();
    return m_pps.data();
}

H265StreamSource* H265StreamSource::createNew(UsageEnvironment& env, int vencChn)
{
    return new H265StreamSource(env, vencChn);
}

H265StreamSource::H265StreamSource(UsageEnvironment& env, int vencChn)
    : FramedSource(env)
    , m_vencChn(vencChn)
    , m_active(false)
{
    pthread_mutex_init(&m_mutex, nullptr);
    pthread_cond_init(&m_cond, nullptr);
}

H265StreamSource::~H265StreamSource()
{
    pthread_mutex_lock(&m_mutex);
    m_active = false;
    pthread_cond_broadcast(&m_cond);
    pthread_mutex_unlock(&m_mutex);

    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);
}

void H265StreamSource::feedFrame(const uint8_t* pData, uint32_t u32Len,
                                  uint64_t u64Pts, bool bKeyFrame)
{
    if (!pData || u32Len == 0) {
        return;
    }

    pthread_mutex_lock(&m_mutex);

    if (m_vps.empty() || m_sps.empty() || m_pps.empty()) {
        uint32_t i = 0;
        while (i + 4 < u32Len) {
            if (pData[i] == 0x00 && pData[i+1] == 0x00 &&
                pData[i+2] == 0x00 && pData[i+3] == 0x01) {
                uint32_t ns  = i + 4;
                uint8_t  nt  = (pData[ns] >> 1) & 0x3F;
                uint32_t ne  = u32Len;
                for (uint32_t j = ns + 1; j + 3 < u32Len; j++) {
                    if (pData[j] == 0x00 && pData[j+1] == 0x00 &&
                        pData[j+2] == 0x00 && pData[j+3] == 0x01) {
                        ne = j;
                        break;
                    }
                }
                if (nt == 32)       m_vps.assign(pData + ns, pData + ne);
                else if (nt == 33)  m_sps.assign(pData + ns, pData + ne);
                else if (nt == 34)  m_pps.assign(pData + ns, pData + ne);
                i = ne;
            } else {
                i++;
            }
        }
    }

    if (m_frameQueue.size() < STREAM_FRAME_QUEUE_MAX) {
        StreamFrame frame;
        frame.data.assign(pData, pData + u32Len);
        frame.pts      = u64Pts;
        frame.keyFrame = bKeyFrame;
        m_frameQueue.push(std::move(frame));
        pthread_cond_signal(&m_cond);
    }

    pthread_mutex_unlock(&m_mutex);
}

void H265StreamSource::doGetNextFrame()
{
    m_active = true;

    pthread_mutex_lock(&m_mutex);

    while (m_frameQueue.empty() && m_active) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += 40 * 1000000;
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000L;
        }
        pthread_cond_timedwait(&m_cond, &m_mutex, &ts);
    }

    if (!m_active || m_frameQueue.empty()) {
        pthread_mutex_unlock(&m_mutex);
        nextTask() = envir().taskScheduler().scheduleDelayedTask(
            0, (TaskFunc*)afterGetting, this);
        return;
    }

    StreamFrame frame = std::move(m_frameQueue.front());
    m_frameQueue.pop();
    pthread_mutex_unlock(&m_mutex);

    if (frame.data.size() > fMaxSize) {
        fFrameSize         = fMaxSize;
        fNumTruncatedBytes = frame.data.size() - fMaxSize;
    } else {
        fFrameSize         = frame.data.size();
        fNumTruncatedBytes = 0;
    }

    memcpy(fTo, frame.data.data(), fFrameSize);
    fPresentationTime.tv_sec  = (long)(frame.pts / 1000000);
    fPresentationTime.tv_usec = (long)(frame.pts % 1000000);
    fDurationInMicroseconds   = 0;

    afterGetting(this);
}

void H265StreamSource::doStopGettingFrames()
{
    m_active = false;
    pthread_mutex_lock(&m_mutex);
    pthread_cond_broadcast(&m_cond);
    pthread_mutex_unlock(&m_mutex);
}

const uint8_t* H265StreamSource::getVps(size_t& len) const
{
    if (m_vps.empty()) { len = 0; return nullptr; }
    len = m_vps.size();
    return m_vps.data();
}

const uint8_t* H265StreamSource::getSps(size_t& len) const
{
    if (m_sps.empty()) { len = 0; return nullptr; }
    len = m_sps.size();
    return m_sps.data();
}

const uint8_t* H265StreamSource::getPps(size_t& len) const
{
    if (m_pps.empty()) { len = 0; return nullptr; }
    len = m_pps.size();
    return m_pps.data();
}
