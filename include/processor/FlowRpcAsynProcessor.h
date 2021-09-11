#pragma once
#include <glog/logging.h>

#include <mutex>
#include <queue>

#include "DynamicFactory.h"
#include "mgr/ConfigureManager.h"
#include "processor/Processor.h"

namespace whale {
namespace vision {

class FlowRpcAsynProcessor : public Processor,
                             DynamicCreator<FlowRpcAsynProcessor> {
 public:
    explicit FlowRpcAsynProcessor() : Processor(WHALE_PROCESSOR_FLOWRPC){};
    virtual ~FlowRpcAsynProcessor() { sendMetric(); }

    void SendToFlowServer(int type, const std::string& eventStr);

    virtual void run();
    virtual void init() { LOG(INFO) << "flow client init"; }

 private:
    void sendMetric();

    // send flow data to remote http chunnel
    bool sendMessage(const std::string& data);
    // save trans failed data to disk
    void saveCache(const std::string& cache_path, const std::string& data);
    // resend disk data to remote
    void resendCache(const std::string& cache_path);

 private:
    std::queue<std::string> q_;
    std::mutex qmt_;
    std::mutex resmt_;

    // metrics
    int cv_flow_rt_upload_failed_count_ = 0;  // 实时数据上传失败次数
    int cv_flow_reupload_success_count_ = 0;  // 重传成功的
    int cv_flow_reupload_retry_count_ = 0;    // 重传失败次数
    int cv_flow_history_reupload_count_ = 0;  // 历史重传失败
    bool is_dev_ = false;
};

// TODO: tmp variable
extern FlowRpcAsynProcessor* FlowRpcAsynProcessor_flow;

}  // namespace vision
}  // namespace whale