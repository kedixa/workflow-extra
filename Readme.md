# WorkflowExtra

## PipeRedisTask
`Redis`任务的`pipeline`模式，一次性向`Redis`发送多条命令，可以提高吞吐量。由于`pipeline`并不是事务，该客户端只是如实地将所有请求发出去，同时接收到所有回复，故一致性等问题不做保证。

示例 `example/PipeRedisExample.cc`展示了使用方法，基本与`WFRedisTask`一致，只是拿到的结果一定是一个`array`，其长度应当与发送的请求数量一致，其中每个元素是对应请求的响应。

