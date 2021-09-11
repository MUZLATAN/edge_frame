
#include "processor/DispatchProcessor.h"


#include "mgr/QueueManager.h"

namespace whale {
namespace vision {

void DispatchProcessor::init() {
    QueueManager::SafeGet(processor_name_, input_queue_);
}

void DispatchProcessor::dispatch(std::shared_ptr<Event> event) {
    if (event->et == EventType::kFrontEvent) {

        std::thread process_t([event](){
            event->handler();
        });
        process_t.detach();
    } else if (event->et == EventType::kVehicleEvent) {

        std::thread process_t([event](){
            event->handler();
        });
        process_t.detach();
    } else {
        event->handler();
    }
}
void DispatchProcessor::run() {
    while (true) {
        if (gt->sys_quit) {
            break;
        }

        // will block this if no event
        boost::any message;
        if (input_queue_->Empty()) {
            sleep(100);
            continue;
        }
        input_queue_->Pop(message);
        std::shared_ptr<Event> event =
            boost::any_cast<std::shared_ptr<Event>>(message);
        dispatch(event);
    }
    LOG(INFO) << processor_name_ << " Exit .......";
}
}  // namespace vision
}  // namespace whale