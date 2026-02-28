#ifndef __LIVE_MEDIA_HH__
#define __LIVE_MEDIA_HH__

/* Live555 stub header for host/stub builds.
 * Replace with real Live555 headers when LIVE555_AVAILABLE is defined.
 */

#ifndef LIVE555_AVAILABLE

#include "UsageEnvironment.hh"
#include <stdint.h>
#include <stddef.h>
#include <sys/time.h>

class Medium {
public:
    static void close(Medium* m) { delete m; }
    virtual ~Medium() {}
};

class FramedSource : public Medium {
public:
    unsigned char* fTo;
    unsigned       fMaxSize;
    unsigned       fFrameSize;
    unsigned       fNumTruncatedBytes;
    struct timeval fPresentationTime;
    unsigned       fDurationInMicroseconds;

    TaskToken& nextTask() { static TaskToken t = nullptr; return t; }
    UsageEnvironment& envir() { return *m_env; }
    static void afterGetting(FramedSource* source) {}

protected:
    explicit FramedSource(UsageEnvironment& env)
        : fTo(nullptr)
        , fMaxSize(0)
        , fFrameSize(0)
        , fNumTruncatedBytes(0)
        , fDurationInMicroseconds(0)
        , m_env(&env)
    {
        fPresentationTime.tv_sec  = 0;
        fPresentationTime.tv_usec = 0;
    }
    virtual void doGetNextFrame() = 0;
    virtual void doStopGettingFrames() {}

private:
    UsageEnvironment* m_env;
};

class ServerMediaSubsession : public Medium {};

class PassiveServerMediaSubsession : public ServerMediaSubsession {
public:
    static PassiveServerMediaSubsession* createNew(Medium& sink,
                                                    FramedSource* src) {
        return new PassiveServerMediaSubsession();
    }
};

class ServerMediaSession : public Medium {
public:
    static ServerMediaSession* createNew(UsageEnvironment& env,
                                          const char* name,
                                          const char* info,
                                          const char* desc) {
        return new ServerMediaSession();
    }
    void addSubsession(ServerMediaSubsession* sub) { delete sub; }
    unsigned referenceCount() const { return 0; }
};

class RTPSink : public Medium {};

class H264VideoRTPSink : public RTPSink {
public:
    static H264VideoRTPSink* createNew(UsageEnvironment& env,
                                        void* groupsock,
                                        unsigned char pt) {
        return new H264VideoRTPSink();
    }
};

class H265VideoRTPSink : public RTPSink {
public:
    static H265VideoRTPSink* createNew(UsageEnvironment& env,
                                        void* groupsock,
                                        unsigned char pt) {
        return new H265VideoRTPSink();
    }
};

class UserAuthenticationDatabase {
public:
    void addUserRecord(const char* user, const char* pass) {}
};

class RTSPServer : public Medium {
public:
    static RTSPServer* createNew(UsageEnvironment& env,
                                  unsigned short port,
                                  UserAuthenticationDatabase* auth,
                                  unsigned reclamationTestSeconds) {
        return new RTSPServer();
    }
    void addServerMediaSession(ServerMediaSession* sms) { delete sms; }
    void removeServerMediaSession(const char* name) {}
    ServerMediaSession* lookupServerMediaSession(const char* name) {
        return nullptr;
    }
    char* rtspURL(ServerMediaSession* sms) const {
        char* url = new char[64];
        url[0] = '\0';
        return url;
    }
};

#endif /* !LIVE555_AVAILABLE */
#endif /* __LIVE_MEDIA_HH__ */
