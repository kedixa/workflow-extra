#include <stdio.h>
#include <string>

#include "workflow/WFTaskError.h"
#include "workflow/WFTaskFactory.h"
#include "workflow/StringUtil.h"

#include "PipeRedisMessage.h"
#include "WETaskFactory.h"

using namespace protocol;

#define REDIS_KEEPALIVE_DEFAULT  (60 * 1000)

/**********Client**********/

class ComplexPipeRedisTask : public WFComplexClientTask<PipeRedisRequest, PipeRedisResponse> {
 public:
    ComplexPipeRedisTask(int retry_max, piperedis_callback_t&& callback):
        WFComplexClientTask(retry_max, std::move(callback)),
        db_num_(0),
        is_user_request_(true),
        redirect_count_(0)
    {}

 protected:
    virtual bool check_request();
    virtual CommMessageOut *message_out();
    virtual CommMessageIn *message_in();
    virtual int keep_alive_timeout();
    virtual bool init_success();
    virtual bool finish_once();

 private:
    bool need_redirect();

    std::string password_;
    int db_num_;
    bool succ_;
    bool is_user_request_;
    int redirect_count_;
};

bool ComplexPipeRedisTask::check_request() {
    for (const std::string& command : this->req.get_commands()) {
        if ((strcasecmp(command.c_str(), "AUTH") == 0 ||
                strcasecmp(command.c_str(), "SELECT") == 0 ||
                strcasecmp(command.c_str(), "ASKING") == 0)) {
            this->state = WFT_STATE_TASK_ERROR;
            this->error = WFT_ERR_REDIS_COMMAND_DISALLOWED;
            return false;
        }
    }

    return true;
}

CommMessageOut *ComplexPipeRedisTask::message_out() {
    long long seqid = this->get_seq();

    if (seqid <= 1) {
        if (seqid == 0 && !password_.empty()) {
            succ_ = false;
            is_user_request_ = false;
            auto *auth_req = new RedisRequest;

            auth_req->set_request("AUTH", {password_});
            return auth_req;
        }

        if (db_num_ > 0 && (seqid == 0 || !password_.empty())) {
            succ_ = false;
            is_user_request_ = false;
            auto *select_req = new RedisRequest;
            char buf[32];

            sprintf(buf, "%d", db_num_);
            select_req->set_request("SELECT", {buf});
            return select_req;
        }
    }

    this->get_resp()->set_result_size(this->get_req()->get_request_size());

    return this->WFComplexClientTask::message_out();
}

CommMessageIn *ComplexPipeRedisTask::message_in() {
    PipeRedisRequest *req = this->get_req();
    PipeRedisResponse *resp = this->get_resp();

    if (is_user_request_)
        resp->set_asking(req->is_asking());
    else
        resp->set_asking(false);

    return this->WFComplexClientTask::message_in();
}

int ComplexPipeRedisTask::keep_alive_timeout() {
    if (this->is_user_request_)
        return this->keep_alive_timeo;

    PipeRedisResponse *resp = this->get_resp();

    succ_ = (resp->parse_success());

    return succ_ ? REDIS_KEEPALIVE_DEFAULT : 0;
}

bool ComplexPipeRedisTask::init_success() {
    TransportType type;

    if (uri_.scheme && strcasecmp(uri_.scheme, "redis") == 0)
        type = TT_TCP;
    else if (uri_.scheme && strcasecmp(uri_.scheme, "rediss") == 0)
        type = TT_TCP_SSL;
    else {
        this->state = WFT_STATE_TASK_ERROR;
        this->error = WFT_ERR_URI_SCHEME_INVALID;
        return false;
    }

    if (uri_.userinfo && uri_.userinfo[0] == ':' && uri_.userinfo[1]) {
        password_.assign(uri_.userinfo + 1);
        StringUtil::url_decode(password_);
    }

    if (uri_.path && uri_.path[0] == '/' && uri_.path[1])
        db_num_ = atoi(uri_.path + 1);

    size_t info_len = password_.size() + 32 + 16;
    char *info = new char[info_len];

    sprintf(info, "redis-pipeline|pass:%s|db:%d", password_.c_str(), db_num_);
    this->WFComplexClientTask::set_transport_type(type);
    this->WFComplexClientTask::set_info(info);

    delete []info;
    return true;
}

bool ComplexPipeRedisTask::need_redirect() {
    // PipeRedis Do Not Support Redirect
    return false;
}

bool ComplexPipeRedisTask::finish_once() {
    if (!is_user_request_) {
        is_user_request_ = true;
        delete this->get_message_out();

        if (this->state == WFT_STATE_SUCCESS) {
            if (succ_)
                this->clear_resp();
            else {
                this->disable_retry();
                this->state = WFT_STATE_TASK_ERROR;
                this->error = WFT_ERR_REDIS_ACCESS_DENIED;
            }
        }
        return false;
    }

    if (this->state == WFT_STATE_SUCCESS) {
        if (need_redirect())
            this->set_redirect(uri_);
        else if (this->state != WFT_STATE_SUCCESS)
            this->disable_retry();
    }

    return true;
}

/**********Factory**********/
// redis://:password@host:port/db_num
WEPipeRedisTask *WETaskFactory::create_piperedis_task(
    const std::string& url,
    int retry_max,
    piperedis_callback_t callback)
{
    auto *task = new ComplexPipeRedisTask(retry_max, std::move(callback));
    ParsedURI uri;

    URIParser::parse(url, uri);
    task->init(std::move(uri));
    task->set_keep_alive(REDIS_KEEPALIVE_DEFAULT);
    return task;
}

WEPipeRedisTask *WETaskFactory::create_piperedis_task(
    const ParsedURI& uri,
    int retry_max,
    piperedis_callback_t callback)
{
    auto *task = new ComplexPipeRedisTask(retry_max, std::move(callback));

    task->init(uri);
    task->set_keep_alive(REDIS_KEEPALIVE_DEFAULT);
    return task;
}

