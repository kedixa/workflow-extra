# WorkflowExtra
本项目为 [C++ Workflow](https://github.com/sogou/workflow) 的外传，用于探索新颖的辅助组件、奇特的使用方式等。本项目实现的接口可能随时被修改、移动、删除，功能可能随时修改含义，请知悉。

## PipeRedisTask
`Redis`任务的`pipeline`模式，一次性向`Redis`发送多条命令，可以提高吞吐量。由于`pipeline`并不是事务，该客户端只是如实地将所有请求发出去，同时接收到所有回复，故一致性等问题不做保证。

示例 [PipeRedisExample](example/PipeRedisExample.cc)展示了使用方法，基本与`WFRedisTask`一致，只是拿到的结果一定是一个`array`，其长度应当与发送的请求数量一致，其中每个元素是对应请求的响应。

## QpsPool
`QpsPool`用于限制每秒钟被发起的任务数量，即使同一时刻启动了多个任务，也会等待到合适的时机再执行。

示例 [QpsPoolExample](example/QpsPoolExample.cc)

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

## Latch
`Latch`用于实现`Series`间的同步，将`Latch::get`创建的任务放置到`Series`的特定位置，当所有`Series`都执行到这个位置时，后续的任务才会被执行。使用`Latch::get(false)`指定该任务不进行等待，该任务仅通知到达事件，不会阻塞当前`Series`。

示例 [Latch](example/LatchExample.cc)

## GoPipe
`GoPipe`是一个将一组`GoTask`串联起来的语法糖，支持串普通函数、可调用对象、lambda、std::function，前一个函数的返回值为后一个函数的参数，若前一个函数无返回值则后一个函数不可有参数。串联后的整个流程运行于同一个`SeriesWork`中，且运行时不可新增或移除任务，是一个编译时的静态流程，故目前有较大的局限性。

示例 [GoPipe](example/GoPipeExample.cc)

```cpp
wfextra::GoPipe<int>()
.then(&pool, func)
.then(double_to_string)
.then(Reverse{})
.then(&single, [](const string &y) {
    cout << "result: " << y << endl;
}).then(std::bind([](int i){ return i; }, 1))
.then([](int){})
.start(i + 1, [&wg]() { wg.done(); });
```
