#ifndef WE_LATCH_H
#define WE_LATCH_H

#include <functional>
#include <mutex>
#include <set>

#include "workflow/WFTask.h"

namespace wfextra {

class LatchManager;

class LatchTask : public WFCounterTask {
public:
    LatchTask(LatchManager *m, bool wait)
        : WFCounterTask(1, nullptr), m(m), wait(wait) { }
    virtual ~LatchTask() { }

protected:
    virtual void dispatch() override;

private:
    LatchManager *m;
    bool wait;
};

class LatchManager {
public:
    LatchManager() : cnt(0) { }

    WFGenericTask *get(bool wait) {
        LatchTask *t = new LatchTask(this, wait);
        tset.insert(t);
        ++cnt;

        return t;
    }

private:
    friend class LatchTask;
    void arrived(LatchTask *cur, bool wait) {
        bool last = false;

        {
            std::lock_guard<std::mutex> lg(mtx);
            last = (--cnt == 0);

            if (!wait)
                tset.erase(cur);
        }

        if (!wait)
            cur->count();

        if (last) {
            for (LatchTask *t : tset)
                t->count();

            delete this;
        }
    }

    std::mutex mtx;
    size_t cnt;
    std::set<LatchTask *> tset;
};

void LatchTask::dispatch() {
    m->arrived(this, wait);
    WFCounterTask::dispatch();
}

class Latch {
public:
    Latch() : m(nullptr) { }
    Latch(const Latch &) = delete;
    ~Latch() { }

    WFGenericTask *get(bool wait) {
        return manager()->get(wait);
    }

    void release() {
        m = nullptr;
    }

private:
    LatchManager *manager() {
        if (!m)
            m = new LatchManager();
        return m;
    }

private:
    LatchManager *m;
};

} // namespace wfextra

using WELatch = wfextra::Latch;

#endif // WE_LATCH_H
