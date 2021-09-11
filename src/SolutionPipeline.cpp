
#include "SolutionPipeline.h"

#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/executors/GlobalExecutor.h>
#include <folly/executors/IOThreadPoolExecutor.h>
#include <folly/futures/Future.h>
#include <folly/init/Init.h>
#include <glog/logging.h>

#include <boost/bind.hpp>
#include <boost/signals2.hpp>
#include <functional>

#include "mgr/ConfigureManager.h"
#include "processor/DataLoaderProcessor.h"
#include "processor/head.h"

#ifdef __WITH_HEAPPROFILE__
#define __MAX_IMAGE_QUEUE_SIZE__ 1  // max ffmpeg queue size
#else
#define __MAX_IMAGE_QUEUE_SIZE__ 10  // max ffmpeg queue size
#endif

namespace whale {
namespace vision {

//WHALE_PROCESSOR_HTTPSERVICE
//WHALE_PROCESSOR_WEBSOCKETSTREAM
std::vector<std::string> SolutionPipeline::global_executor_names_ = {
// #ifndef __PLATFORM_HISI__
//     , ,
// #endif
    WHALE_PROCESSOR_DISPATCH, WHALE_PROCESSOR_FLOWRPC};

std::unordered_map<std::string, std::shared_ptr<Processor>>
    SolutionPipeline::global_executors_;

std::vector<std::string> SolutionPipeline::global_core_executor_names_ = {
    WHALE_PROCESSOR_DETECT};

std::unordered_map<std::string, std::shared_ptr<Processor>>
    SolutionPipeline::global_core_executors_;

std::vector<std::string> SolutionPipeline::global_video_input_;
std::unordered_map<std::string, std::shared_ptr<Processor>>
    SolutionPipeline::global_video_input_executors_;

std::vector<std::shared_ptr<std::thread>> SolutionPipeline::vec_thread;

template <typename... Targs>
static std::shared_ptr<Processor> buildProcessor(std::string& name,
                                                 const std::string& next,
                                                 Targs&&... params) {
    std::string processor_name = "whale::vision::" + name;
    LOG(INFO) << " load processor: " << processor_name;
    auto ptr = ProcessorBuild::MakeSharedProcessor(processor_name, params...);

    if (!ptr) {
        // todo log
        //*todo crash log
        LOG(FATAL) << processor_name
                   << " dynamic create error because the processor don't "
                      "register, user don't implement the processor.";
    }

    ptr->setNextProcessor(next);

    LOG(INFO) << " this processor queue_name: " << name;
    auto queue = std::make_shared<Queue<boost::any>>(__MAX_IMAGE_QUEUE_SIZE__);

    QueueManager::SafeAdd(name, queue);

    return ptr;
}

void SolutionPipeline::Build() {
    std::vector<std::string> solution_process_vec = feature_map[solution_name_];
    for (int i = 0; i < solution_process_vec.size(); ++i) {
        std::string next;
        if (i != solution_process_vec.size() - 1) {
            next = solution_process_vec[i + 1];
        } else {
            LOG(INFO) << "solution last processor: " << solution_process_vec[i];
            next = WHALE_PROCESSOR_DISPATCH;
        }

        auto ptr = buildProcessor(solution_process_vec[i], next);
        executors_.emplace_back(ptr);
        ptr->init();
    }
}

void SolutionPipeline::BuildVideoInput() {
    LOG(INFO) << "build video input.";
    global_video_input_ =
        ConfigureManager::instance()->getAsVectorString("video_input");

    if (global_video_input_.size() < 1) {
        LOG(FATAL) << " solution don't assigin video/rtst stream.";
        // todo log to chunnel
    }

    for (auto iter = global_video_input_.begin();
         iter != global_video_input_.end(); ++iter) {
        std::string data_loader_name;
        // todo may be local video file
        if ((*iter).find("rtsp") != (*iter).npos) {
            // rtsp
            data_loader_name = WHALE_PROCESSOR_RTSP;
        } else if ((*iter).find("/dev") != (*iter).npos) {
            // usb
            data_loader_name = WHALE_PROCESSOR_USBCAMERA;

        } else if ((*iter).find("rtmp") != (*iter).npos) {
            // rtmp
            LOG(INFO) << "build rtmp input.";
        } else if (*iter == "hisi") {
            // hisi
            data_loader_name = WHALE_PROCESSOR_HISICAMERA;
            LOG(INFO) << "hisi input.";
        } else {
            // local video file
            data_loader_name = WHALE_PROCESSOR_LOCALVIDEO;

            GetGlobalVariable()->isLocalInput = true;
        }

        auto ptr =
            buildProcessor(data_loader_name, WHALE_PROCESSOR_DETECT, *iter);
        global_video_input_executors_[*iter] = ptr;
    }
}

void SolutionPipeline::BuildAndStartCoreGlobal() {
    for (auto iter = global_core_executor_names_.begin();
         iter != global_core_executor_names_.end(); iter++) {
        std::string processor_name = "whale::vision::" + (*iter);
        LOG(INFO) << " core processor: " << processor_name;

        std::string next;
        auto ptr = buildProcessor(*iter, next);

        global_core_executors_[*iter] = ptr;

        if (*iter == WHALE_PROCESSOR_DETECT) {
            // set next processor
        }

        ptr->init();

        folly::via(folly::getCPUExecutor().get()).thenValue([ptr](auto&&) {
            folly::setThreadName(ptr->getProcessorName());
            LOG(INFO) << ptr->getProcessorName() << " start run.";
            return ptr->run();
        });
    }
}

void SolutionPipeline::BuildAndStartGlobal() {
    std::vector<std::string> features =
        ConfigureManager::instance()->getAsVectorString("feature");

    for (auto iter = global_executor_names_.begin();
         iter != global_executor_names_.end(); iter++) {
        std::string next;
        auto ptr = buildProcessor(*iter, next);

        global_executors_[*iter] = ptr;

        ptr->init();
        folly::via(folly::getCPUExecutor().get()).thenValue([ptr](auto&&) {
            folly::setThreadName(ptr->getProcessorName());
            LOG(INFO) << ptr->getProcessorName() << " start run.";
            return ptr->run();
        });
    }

    // init global callback function
    auto processor =
        SolutionPipeline::global_executors_[WHALE_PROCESSOR_FLOWRPC];
    FlowRpcAsynProcessor_flow =
        dynamic_cast<FlowRpcAsynProcessor*>(processor.get());
}

void SolutionPipeline::Start() {
    for (auto iter = executors_.begin(); iter != executors_.end(); ++iter) {
        LOG(INFO) << " thread start: " << (*iter)->getProcessorName();
        folly::via(folly::getCPUExecutor().get()).thenValue([iter](auto&&) {
            folly::setThreadName((*iter)->getProcessorName());
            return (*iter)->run();
        });
//        ptr_thread->detach();
//        vec_thread.emplace_back(ptr_thread);
    }
}

void SolutionPipeline::StopAll() { GetGlobalVariable()->sys_quit = true; }

void SolutionPipeline::StartVideoInput() {
    for (auto iter = global_video_input_executors_.begin();
         iter != global_video_input_executors_.end(); ++iter) {
        LOG(INFO) << "video input thread start: "
                  << iter->second->getProcessorName();
        folly::via(folly::getCPUExecutor().get()).thenValue([iter](auto&&) {
            iter->second->init();
            folly::setThreadName(iter->second->getProcessorName());
            return iter->second->run();
        });
//        ptr_thread->detach();
//        vec_thread.emplace_back(ptr_thread);
    }
}
}  // namespace vision
}  // namespace whale
