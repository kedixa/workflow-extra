#ifndef WE_QPS_POOL_H
#define WE_QPS_POOL_H

#include <mutex>
#include "workflow/WFTask.h"

class WEQpsPool {
public:
    WEQpsPool(unsigned qps = 0);

    void set_qps(unsigned qps);
    SubTask *get(SubTask *task, size_t cnt = 1);

    friend class WEQpsTask;

protected:
    virtual long long get_wait_nano(size_t cnt);
    virtual void done(size_t cnt) { }

private:
    std::mutex mtx;
    long long interval_nano;
    long long last_nano;
};

#endif // WE_QPS_POOL_H
