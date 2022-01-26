#ifndef WE_QPS_POOL_H
#define WE_QPS_POOL_H

#include <mutex>
#include "workflow/WFTask.h"

class WEQpsPool {
public:
    WEQpsPool(unsigned qps = 0);

    void set_qps(unsigned qps);
    SubTask *get(SubTask *task);

    friend class WEQpsTask;

protected:
    virtual long long get_wait_nano();
    virtual void done() { }

private:
    std::mutex mtx;
    long long interval_nano;
    long long last_nano;
};

#endif // WE_QPS_POOL_H
