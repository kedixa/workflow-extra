# WorkflowExtra
本项目为 [C++ Workflow](https://github.com/sogou/workflow) 的外传，用于探索新颖的辅助组件、奇特的使用方式等。本项目实现的接口可能随时被修改、移动、删除，功能可能随时修改含义，请知悉。

## PipeRedisTask
`Redis`任务的`pipeline`模式，一次性向`Redis`发送多条命令，可以提高吞吐量。由于`pipeline`并不是事务，该客户端只是如实地将所有请求发出去，同时接收到所有回复，故一致性等问题不做保证。

示例 `example/PipeRedisExample.cc`展示了使用方法，基本与`WFRedisTask`一致，只是拿到的结果一定是一个`array`，其长度应当与发送的请求数量一致，其中每个元素是对应请求的响应。

## QpsPool
QpsPool用于限制每秒钟被发起的任务数量，即使同一时刻启动了多个任务，也会等待到合适的时机再执行。

```cpp
WEQpsPool pool(10); // 最大Qps为10
WFFacilities::WaitGroup wg(100);

 // 同时发起100个任务
for (int i = 0; i < 100; i++) {
    auto *task = WFTaskFactory::create_go_task("", [&]() {
        wg.done();
    });
    // 将任务托管给QpsPool
    Workflow::start_series_work(pool.get(task), nullptr);
}

wg.wait();
```
