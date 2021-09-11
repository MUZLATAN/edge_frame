#pragma once

#include <chrono>
#include <thread>
#include "DynamicFactory.h"
#include "processor/Processor.h"
#include "event/Event.h"

namespace whale {
namespace vision {
class Process;

class DispatchProcessor : public Processor, DynamicCreator<DispatchProcessor> {
 public:
	DispatchProcessor() : Processor(WHALE_PROCESSOR_DISPATCH) {}
	virtual ~DispatchProcessor() {}
	virtual void run();

	virtual void init();
	void dispatch(std::shared_ptr<Event> event_ptr);
};
}	 // namespace vision
}	 // namespace whale