#ifndef __USAGE_ENVIRONMENT_HH__
#define __USAGE_ENVIRONMENT_HH__

/* Live555 stub header for host/stub builds. */

#ifndef LIVE555_AVAILABLE

#include <stdint.h>

typedef void TaskFunc(void* clientData);
typedef void* TaskToken;

class TaskScheduler {
public:
    virtual ~TaskScheduler() {}
    void doEventLoop(volatile bool* watch) {}
    TaskToken scheduleDelayedTask(int64_t usDelay, TaskFunc* proc,
                                   void* clientData) {
        return nullptr;
    }
    void unscheduleDelayedTask(TaskToken& task) {}
};

class UsageEnvironment {
public:
    virtual ~UsageEnvironment() {}
    TaskScheduler& taskScheduler() { return *m_scheduler; }

protected:
    explicit UsageEnvironment(TaskScheduler& sched) : m_scheduler(&sched) {}

private:
    TaskScheduler* m_scheduler;
};

#endif /* !LIVE555_AVAILABLE */
#endif /* __USAGE_ENVIRONMENT_HH__ */
