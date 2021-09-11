
#include "processor/FlowRpcAsynProcessor.h"

#include <json/json.h>
#include <fstream>


#include "ServiceMetricManager.h"
#include "SysPublicTool.h"
#include "VideoRecorder.h"
#include "common.h"
#include "mgr/ConfigureManager.h"
#include "network/HttpClient.h"

namespace whale {
namespace vision {
FlowRpcAsynProcessor* FlowRpcAsynProcessor_flow;

void FlowRpcAsynProcessor::SendToFlowServer(int type,
                                            const std::string& eventStr) {
    Json::Value kafka_pack;
    kafka_pack["type"] = type;
    kafka_pack["data"] = eventStr;
    kafka_pack["key"] = gt->client_sn;
    qmt_.lock();
    if (q_.size() > 2000) {
        std::string data_store =
            ConfigureManager::instance()->getAsString("sys_path") + "/data/";
        saveCache(data_store, Json::FastWriter().write(kafka_pack));
    } else
        q_.push(Json::FastWriter().write(kafka_pack));
    if (q_.size() == 1) resmt_.unlock();
    qmt_.unlock();
}

void FlowRpcAsynProcessor::sendMetric() {
    // send to chunnel for monitor
    if (cv_flow_rt_upload_failed_count_ || cv_flow_reupload_success_count_ ||
        cv_flow_reupload_retry_count_ || cv_flow_history_reupload_count_) {
        Json::Value data;

        data["time"] = getCurrentTime();
        data["action"] = static_cast<int>(MetricAction::kFlowStatusMonitor);

        data["cv_flow_rt_upload_failed_count"] =
            cv_flow_rt_upload_failed_count_;
        data["cv_flow_reupload_success_count"] =
            cv_flow_reupload_success_count_;
        data["cv_flow_reupload_retry_count"] = cv_flow_reupload_retry_count_;
        data["cv_flow_history_reupload_count"] =
            cv_flow_history_reupload_count_;
//        SendToChunnel(data);

        cv_flow_rt_upload_failed_count_ = 0;
        cv_flow_reupload_success_count_ = 0;
        cv_flow_reupload_retry_count_ = 0;
        cv_flow_history_reupload_count_ = 0;
    }
}

bool FlowRpcAsynProcessor::sendMessage(const std::string& data) {
    if (data.size() == 0) {
        LOG(INFO) << "NOTE: message size must big than zero";
        return true;
    }
    std::string response;
    return HttpPost(GetGlobalVariable()->flowrpc_url.c_str(), data, response);
}

void FlowRpcAsynProcessor::saveCache(const std::string& cache_path,
                                     const std::string& data) {
    std::string name = cache_path + std::to_string(getCurrentTime()) +
                       "_request_grpc_flow_failed.dat";
    std::ofstream fout(name);
    LOG(INFO) << "save to cache: " << data;
    fout << data;
    fout.close();
    addCompletionStamp(name);
};

void FlowRpcAsynProcessor::resendCache(const std::string& cache_path) {
    auto filelist = SysPublicTool::searchFile(
        cache_path, "(.*)(flow_failed.dat" + getCompletionStamp() + ")");
    cv_flow_history_reupload_count_ += filelist.size();
    while (filelist.size() > 0) {
        LOG(INFO) << "flow: resend file " << filelist.front();
        auto ret = sendMessage(readFile(cache_path + filelist.front()));
        if (!ret) {
            LOG(INFO) << "NOTE: resend flow cache failed ";
            cv_flow_reupload_retry_count_++;
            return;
        }
        cv_flow_reupload_success_count_++;
        // rm cache
        if (remove((cache_path + filelist.front()).c_str()) != 0) {
            LOG(ERROR) << "cannot remove file:"
                       << cache_path + filelist.front();
            perror("remove");
        }
        filelist.pop();
    }
};

void FlowRpcAsynProcessor::run() {
    auto envStr = ConfigureManager::instance()->getAsString("env");
    if (envStr == "dev") is_dev_ = true;

    std::string data_store =
        ConfigureManager::instance()->getAsString("sys_path") + "/data/";
    // rm unfinished cache
    auto unfi_filelist =
        SysPublicTool::searchFile(data_store, "(.*)(flow_failed.dat)");

    cv_flow_history_reupload_count_ += unfi_filelist.size();
    while (unfi_filelist.size() > 0) {
        LOG(INFO) << "flow: remove unfinished file " << unfi_filelist.front();
        if (remove((data_store + unfi_filelist.front()).c_str()) != 0) {
            LOG(ERROR) << "cannot remove file:"
                       << data_store + unfi_filelist.front();
            perror("remove");
        }
        unfi_filelist.pop();
    }

    resendCache(data_store);

    int metric_upload_freq = 0;
    int continue_failed_cnt = 0;

    std::queue<std::string> failed_q;

    bool ret = false;
    while (true) {
        qmt_.lock();
        resmt_.lock();
        if (q_.size() == 0) {
            qmt_.unlock();
            resmt_.lock();  // thread sleep
            CHECK_GE(q_.size(), 0)
                << " ERROR: q_.size() must be bigger than 0 !!!!";
        } else
            qmt_.unlock();
        resmt_.unlock();

        while (q_.size() > 0) {
            ret = sendMessage(q_.front());
            if (!ret) {
                continue_failed_cnt++;
                failed_q.push(q_.front());
                cv_flow_rt_upload_failed_count_++;
            } else {
                // 由于数据较小，所以可以一旦网络发送成功，那么就可以把堆积的失败数据全部发送
                while (failed_q.size() > 0) {
                    ret = sendMessage(failed_q.front());
                    if (ret) {
                        cv_flow_reupload_retry_count_++;
                        break;
                    } else {
                        cv_flow_reupload_success_count_++;
                        failed_q.pop();
                    }
                }
                if (ret) resendCache(data_store);
            }
            qmt_.lock();
            q_.pop();
            qmt_.unlock();

            if (continue_failed_cnt >= 10) {
                // write to file
                while (failed_q.size() > 0) {
                    saveCache(data_store, failed_q.front());
                    failed_q.pop();
                }
                continue_failed_cnt = 0;
            }

            // when retry success, we send metric.
            if (metric_upload_freq++ % 20 == 0) {
                sendMetric();
                metric_upload_freq = 0;
            }
        }
    }
}
}  // namespace vision
}  // namespace whale