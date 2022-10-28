#include <iostream>
#include <chrono>
#include "workflow/WFFacilities.h"
#include "QpsPool.h"

using namespace std;

int main(int argc, char *argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " tasks qps\n";
        exit(1);
    }

    int tasks = atoi(argv[1]);
    int qps = atoi(argv[2]);

    if (tasks <= 0 || qps <= 0)
        exit(1);

    WEQpsPool pool(qps);
    WFFacilities::WaitGroup wg(tasks);

    auto start = std::chrono::system_clock::now();

    for (int i = 0; i < tasks; i++) {
        auto *task = WFTaskFactory::create_go_task("", [&]() {
            wg.done();
        });
        Workflow::start_series_work(pool.get(task), nullptr);
    }

    wg.wait();

    auto stop = std::chrono::system_clock::now();
    std::chrono::duration<double> d(stop - start);

    cout << "Run " << tasks << " tasks with qps " << qps << "\n"
        << "cost " << d.count() << " seconds\n";
    return 0;
}
