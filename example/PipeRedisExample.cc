#include <string>
#include <vector>
#include <iostream>

#include "workflow/WFFacilities.h"
#include "workflow/StringUtil.h"
#include "TaskFactory.h"
using namespace std;

WFFacilities::WaitGroup wait_group(1);

void redis_callback(WEPipeRedisTask *task) {
    int state = task->get_state();
    int error = task->get_error();

    if (state != WFT_STATE_SUCCESS) {
        const char *err = WFGlobal::get_error_string(state, error);
        cerr << err << endl;
        return;
    }

    protocol::RedisValue result;
    task->get_resp()->get_result(result);

    for (size_t i = 0; i < result.arr_size(); i++) {
        protocol::RedisValue &v = result.arr_at(i);
        cout << v.debug_string() << endl;
    }

    wait_group.done();
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <redis url>\n";
        exit(1);
    }

    std::string redis_url = argv[1];
    if (strncasecmp(argv[1], "redis://", 8) != 0 &&
        strncasecmp(argv[1], "rediss://", 9) != 0)
    {
        redis_url = "redis://" + redis_url;
    }

    WEPipeRedisTask *task = WETaskFactory::create_piperedis_task(
        redis_url, 0, redis_callback);
    protocol::PipeRedisRequest *req = task->get_req();

    std::string line;
    std::string command;
    std::vector<std::string> params;

    while (getline(cin, line)) {
        auto args = StringUtil::split_filter_empty(line, ' ');
        if (args.empty())
            continue;

        command = args[0];
        params.assign(args.begin() + 1, args.end());

        if (params.empty() && strcasecmp("exec", command.c_str()) == 0)
            break;

        req->add_request(command, params);
    }

    task->start();
    wait_group.wait();

    return 0;
}
