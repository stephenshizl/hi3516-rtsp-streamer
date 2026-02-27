#ifndef __H264_STREAM_SOURCE_H__
#define __H264_STREAM_SOURCE_H__

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include <pthread.h>
#include <queue>
#include <vector>

#define STREAM_FRAME_QUEUE_MAX  32
#define STREAM_FRAME_BUF_SIZE   (512 * 1024)

struct StreamFrame {
    std::vector<uint8_t> data;
    uint64_t             pts;
    bool                 keyFrame;
};

class H264StreamSource : public FramedSource {
public:
    static H264StreamSource* createNew(UsageEnvironment& env, int vencChn);
    virtual ~H264StreamSource();

    void feedFrame(const uint8_t* pData, uint32_t u32Len,
                   uint64_t u64Pts, bool bKeyFrame);

    const uint8_t* getSps(size_t& len) const;
    const uint8_t* getPps(size_t& len) const;

protected:
    H264StreamSource(UsageEnvironment& env, int vencChn);
    virtual void doGetNextFrame();
    virtual void doStopGettingFrames();

private:
    void parseSpsPps(const uint8_t* pData, uint32_t u32Len);
    bool findNalu(const uint8_t* pData, uint32_t u32Len,
                  uint32_t& startOff, uint32_t& naluLen);

    int                       m_vencChn;
    std::queue<StreamFrame>   m_frameQueue;
    pthread_mutex_t           m_mutex;
    pthread_cond_t            m_cond;
    bool                      m_hasSps;
    bool                      m_hasPps;
    std::vector<uint8_t>      m_sps;
    std::vector<uint8_t>      m_pps;
    bool                      m_active;
};

class H265StreamSource : public FramedSource {
public:
    static H265StreamSource* createNew(UsageEnvironment& env, int vencChn);
    virtual ~H265StreamSource();

    void feedFrame(const uint8_t* pData, uint32_t u32Len,
                   uint64_t u64Pts, bool bKeyFrame);

    const uint8_t* getVps(size_t& len) const;
    const uint8_t* getSps(size_t& len) const;
    const uint8_t* getPps(size_t& len) const;

protected:
    H265StreamSource(UsageEnvironment& env, int vencChn);
    virtual void doGetNextFrame();
    virtual void doStopGettingFrames();

private:
    int                       m_vencChn;
    std::queue<StreamFrame>   m_frameQueue;
    pthread_mutex_t           m_mutex;
    pthread_cond_t            m_cond;
    bool                      m_active;
    std::vector<uint8_t>      m_vps;
    std::vector<uint8_t>      m_sps;
    std::vector<uint8_t>      m_pps;
};

#endif /* __H264_STREAM_SOURCE_H__ */
