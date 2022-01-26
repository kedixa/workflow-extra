#ifndef WE_PIPE_REDIS_MESSAGE_H
#define WE_PIPE_REDIS_MESSAGE_H
#include "workflow/RedisMessage.h"

namespace protocol {

class PipeRedisRequest : public RedisMessage {
public:
    PipeRedisRequest() = default;
    PipeRedisRequest(PipeRedisRequest&&) = default;
    PipeRedisRequest& operator=(PipeRedisRequest&&) = default;

public:
    void add_request(const std::string& command,
                     const std::vector<std::string>& params);
    void clear_request();

    // Inner
    const std::vector<std::string> &get_commands() const { return commands_; }
    void set_request_size(size_t size) { request_size_ = size; }
    void set_message(const std::string &s) { msg_ = s; }

    size_t get_request_size() const {
        if (request_size_ == 0)
            return commands_.size();

        return request_size_;
    }

protected:
    virtual int encode(struct iovec vectors[], int max);
    virtual int append(const void *buf, size_t *size) { return -1; }

private:
    size_t request_size_{0};
    std::vector<std::string> commands_;
    std::vector<std::vector<std::string>> params_;
    std::string msg_;
};

class PipeRedisResponse : public RedisResponse {
public:
    PipeRedisResponse() : result_size_(0) {}
    PipeRedisResponse(PipeRedisResponse&&) = default;
    PipeRedisResponse& operator=(PipeRedisResponse&&) = default;

public:
    // Inner
    void set_result_size(size_t n);

private:
    size_t result_size_;
};

} // namespace protocol

#endif // WE_PIPE_REDIS_MESSAGE_H
