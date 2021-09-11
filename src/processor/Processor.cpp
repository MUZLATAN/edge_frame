#include "processor/Processor.h"

#include <glog/logging.h>

#include "SolutionPipeline.h"

namespace whale {
namespace vision {
void Processor::init() {
    QueueManager::SafeGet(next_processor_name_, output_queue_);
    if (!output_queue_) LOG(FATAL) << processor_name_ << " Get Queue error.";

    LOG(INFO) << processor_name_ << " base Processor init.. "
              << "current processor: " << processor_name_
              << " Next processor: " << next_processor_name_;
}

}  // namespace vision
}  // namespace whale