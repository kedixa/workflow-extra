#ifndef WE_TASK_FACTORY_H
#define WE_TASK_FACTORY_H

#include "PipeRedisMessage.h"
#include "workflow/WFTask.h"

namespace wfextra {

using PipeRedisTask = WFNetworkTask<protocol::PipeRedisRequest,
                                    protocol::PipeRedisResponse>;
using piperedis_callback_t = std::function<void (PipeRedisTask *)>;

class TaskFactory {
public:
    static PipeRedisTask *create_piperedis_task(
        const std::string& url, int retry_max,
        piperedis_callback_t callback);

    static PipeRedisTask *create_piperedis_task(
        const ParsedURI& uri, int retry_max,
        piperedis_callback_t callback);
};

} // namespace wfextra

using WETaskFactory = wfextra::TaskFactory;
using WEPipeRedisTask = wfextra::PipeRedisTask;
using wfextra::piperedis_callback_t;

#endif // WE_TASK_FACTORY_H
