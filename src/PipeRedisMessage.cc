#include <errno.h>
#include <string.h>
#include <sstream>
#include <utility>

#include "workflow/EncodeStream.h"
#include "PipeRedisMessage.h"

namespace protocol {

void PipeRedisRequest::add_request(const std::string& command,
                                   const std::vector<std::string>& params) {
    commands_.push_back(command);
    params_.push_back(params);
}

int PipeRedisRequest::encode(struct iovec vectors[], int max) {
    if (!msg_.empty()) {
        vectors[0].iov_base = (void *)msg_.c_str();
        vectors[0].iov_len = msg_.size();
        return 1;
    }

    size_t n = commands_.size();
    if (n == 0)
        return 0;

    redis_reply_t *reply = &parser_->reply;
    redis_reply_deinit(reply);
    redis_reply_init(reply);
    redis_reply_set_array(n, reply);

    for (size_t i = 0; i < n; i++) {
        redis_reply_t *r = reply->element[i];
        redis_reply_set_array(params_[i].size() + 1, r);
        redis_reply_set_string(commands_[i].c_str(),
                               commands_[i].size(),
                               r->element[0]);
        for (size_t j = 0; j < params_[i].size(); j++) {
            redis_reply_set_string(params_[i][j].c_str(),
                                   params_[i][j].size(),
                                   r->element[j+1]);
        }
    }

    stream_->reset(vectors, max);

    for (size_t i = 0; i < reply->elements; i++) {
        if (!encode_reply(reply->element[i]))
            return 0;
    }

    return stream_->size();
}

void PipeRedisResponse::set_result_size(size_t n) {
    size_t size;
    std::string message = "*";

    message += std::to_string(n);
    message += "\r\n";
    size = message.size();
    result_size_ = n;

    redis_parser_deinit(parser_);
    redis_parser_init(parser_);
    redis_parser_append_message(message.c_str(), &size, parser_);
}

} // namespace protocol
