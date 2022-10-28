#include <cstdio>
#include <thread>
#include <chrono>

#include "workflow/WFTaskFactory.h"
#include "workflow/WFFacilities.h"
#include "Latch.h"

constexpr size_t PARALLEL_SIZE = 3;

long long data[PARALLEL_SIZE];

void sleep(int sec) {
    std::this_thread::sleep_for(std::chrono::seconds(sec));
}

void prepare_data(int idx) {
    // Simulate time-consuming operations
    sleep(idx);

    long long sum = 0;
    for (int i = idx; i <= 1000000; i += PARALLEL_SIZE)
        sum += i;
    data[idx] = sum;
    printf("prepare %d done\n", idx);
}

void use_data() {
    long long sum = 0;
    for (int i = 0; i < PARALLEL_SIZE; i++)
        sum += data[i];
    printf("use data sum: %lld\n", sum);
}

void deinit_data() {
    for (int i = 0; i < PARALLEL_SIZE; i++)
        data[i] = 0;
    printf("deinit data\n");
}

void anything_else(int x) {
    sleep(1);
    printf("anything else %d\n", x);
}


int main() {
    /*
    TaskGraph:

           / - prepare - 0 - \
     start - - prepare - 1 - - done
           \ - prepare - 2 - /
                       | (prepare latch)
                 start - use data - 3 - done
                 start - use data - 4 - done
                                  | (deinit_latch)
                        start - 5 - deinit data - done
    */

    WFFacilities::WaitGroup wg(1 + 2 + 1);
    WELatch prepare_latch;
    WELatch deinit_latch;

    ParallelWork *par = Workflow::create_parallel_work(
        [&wg](const ParallelWork *) { wg.done(); }
    );
    SeriesWork *use[2], *deinit;

    for (int i = 0; i < PARALLEL_SIZE; i++) {
        WFGoTask *go = WFTaskFactory::create_go_task("", prepare_data, i);
        SeriesWork *series = Workflow::create_series_work(go, nullptr);
        series->push_back(prepare_latch.get(false));  // signal but not wait
        series->push_back(WFTaskFactory::create_go_task("", anything_else, i));
        par->add_series(series);
    }

    for (int i = 0; i < 2; i++) {
        use[i] = Workflow::create_series_work(
            prepare_latch.get(true),                // signal and wait all
            [&wg] (const SeriesWork *) { wg.done(); }
        );
        use[i]->push_back(WFTaskFactory::create_go_task("", use_data));
        use[i]->push_back(deinit_latch.get(false));
        use[i]->push_back(WFTaskFactory::create_go_task("", anything_else, i + 3));
    }

    deinit = Workflow::create_series_work(
        WFTaskFactory::create_go_task("", anything_else, 5),
        [&wg] (const SeriesWork *) { wg.done(); }
    );
    deinit->push_back(deinit_latch.get(true));
    deinit->push_back(WFTaskFactory::create_go_task("", deinit_data));

    // latch.get() cannot be used after any task(created by this latch) started
    // so it is a good practice to release it
    prepare_latch.release();
    deinit_latch.release();

    par->start();
    use[0]->start();
    use[1]->start();
    deinit->start();

    wg.wait();
    return 0;
}