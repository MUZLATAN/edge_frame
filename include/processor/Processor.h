#pragma once

#include <boost/bind.hpp>
#include <chrono>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "common.h"
#include "mgr/QueueManager.h"

namespace whale {
namespace vision {

class Processor {
 public:
    Processor(const std::string& name) : processor_name_(name) {
        gt = GetGlobalVariable();
    }
    virtual ~Processor() { LOG(INFO) << processor_name_ << " exit."; }
    virtual void init();
    virtual void run() = 0;
    virtual void updateConf(){};
    void print() { LOG(INFO) << "print info " << processor_name_; }
    std::string getProcessorName() { return processor_name_; }
    void setNextProcessor(const std::string& netx_processor) {
        next_processor_name_ = netx_processor;
    }
    int64_t getCurrentTime() {
        return std::chrono::system_clock::now().time_since_epoch() /
               std::chrono::milliseconds(1);
    }

 protected:
    std::string processor_name_;
    std::string next_processor_name_;
    QueuePtr input_queue_;
    QueuePtr output_queue_;

    GlobalVariable* gt = nullptr;
};

}  // namespace vision
}  // namespace whale