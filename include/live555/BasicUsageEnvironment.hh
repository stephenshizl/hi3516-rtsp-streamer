#ifndef __BASIC_USAGE_ENVIRONMENT_HH__
#define __BASIC_USAGE_ENVIRONMENT_HH__

/* Live555 stub header for host/stub builds. */

#ifndef LIVE555_AVAILABLE

#include "UsageEnvironment.hh"

class BasicTaskScheduler : public TaskScheduler {
public:
    static BasicTaskScheduler* createNew() {
        return new BasicTaskScheduler();
    }
};

class BasicUsageEnvironment : public UsageEnvironment {
public:
    static BasicUsageEnvironment* createNew(TaskScheduler& sched) {
        return new BasicUsageEnvironment(sched);
    }
    void reclaim() { delete this; }

protected:
    explicit BasicUsageEnvironment(TaskScheduler& sched)
        : UsageEnvironment(sched) {}
};

#endif /* !LIVE555_AVAILABLE */
#endif /* __BASIC_USAGE_ENVIRONMENT_HH__ */
