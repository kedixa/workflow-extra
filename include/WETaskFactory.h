#ifndef WE_TASK_FACTORY_H
#define WE_TASK_FACTORY_H

#include "PipeRedisMessage.h"
#include "workflow/WFTaskFactory.h"

using WEPipeRedisTask = WFNetworkTask<protocol::PipeRedisRequest,
                                      protocol::PipeRedisResponse>;
using piperedis_callback_t = std::function<void (WEPipeRedisTask *)>;

class WETaskFactory {
public:
    static WEPipeRedisTask *create_piperedis_task(
        const std::string& url, int retry_max,
        piperedis_callback_t callback);

    static WEPipeRedisTask *create_piperedis_task(
        const ParsedURI& uri, int retry_max,
        piperedis_callback_t callback);
};

#endif // WE_TASK_FACTORY_H
