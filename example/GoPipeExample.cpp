#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "workflow/WFFacilities.h"
#include "GoPipe.h"

using namespace std;

std::string double_to_string(double d) {
    return to_string(d);
}

struct Reverse {
    std::string operator()(const std::string &s) {
        return std::string(s.rbegin(), s.rend());
    }
};

void sleep(int sec) {
    std::this_thread::sleep_for(std::chrono::seconds(sec));
}

int main() {
    WFFacilities::WaitGroup wg(10);
    WFResourcePool single(1);
    WFResourcePool pool(3);

    std::function<double(int)> func = [](int x) { sleep(1); return x * 3.1415926; };

    for (int i = 0; i < 10; i++) {
        wfextra::GoPipe<int>()
        .then(&pool, func)
        .then(double_to_string)
        .then(Reverse{})
        .then(&single, [](const string &y) {
            cout << "result: " << y << endl;
        }).then(std::bind([](int i){ return i; }, 1))
        .then([](int){})
        .start(i + 1, [&wg]() { wg.done(); });
    }

    wg.wait();
    return 0;
}