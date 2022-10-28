#include <chrono>

#include "QpsPool.h"
#include "workflow/WFTaskFactory.h"

namespace wfextra {

class QpsTask : public WFGenericTask {
public:
    QpsTask(SubTask *task, QpsPool *pool, size_t cnt)
        : task(task), pool(pool), cnt(cnt) { }

protected:
    virtual void dispatch() {
        constexpr long long SEC_MOD = 1000000000;
        long long wait = pool->get_wait_nano(cnt);
        SeriesWork *series = series_of(this);

        if (task) {
            series->push_front(task);
            task = nullptr;
        }

        if (wait > 0) {
            QpsPool *this_pool = pool;
            size_t this_cnt = cnt;
            auto *timer = WFTaskFactory::create_timer_task(wait / SEC_MOD, wait % SEC_MOD,
                [this_pool, this_cnt](WFTimerTask *) {
                    this_pool->done(this_cnt);
            });
            series->push_front(timer);
        }
        else
            pool->done(this->cnt);

        WFGenericTask::dispatch();
    }

private:
    SubTask *task;
    QpsPool *pool;
    size_t cnt;
};

static long long __get_current_nano() {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
}

QpsPool::QpsPool(unsigned qps) {
    last_nano = 0;
    set_qps(qps);
}

void QpsPool::set_qps(unsigned qps) {
    std::lock_guard<std::mutex> lg(mtx);
    if (qps == 0)
        interval_nano = 0;
    else
        interval_nano = 1000000000ULL / qps;
}

SubTask *QpsPool::get(SubTask *task, size_t cnt) {
    return new QpsTask(task, this, cnt);
}

long long QpsPool::get_wait_nano(size_t cnt) {
    std::lock_guard<std::mutex> lg(mtx);
    long long current = __get_current_nano();
    long long cost = interval_nano * cnt;
    long long next = last_nano + cost;
    if (next >= current) {
        last_nano = next;
        return next - current;
    }
    else {
        last_nano = current;
        return 0;
    }
}

} // namespace wfextra
