#pragma once

#include <map>
#include <opencv2/opencv.hpp>

#include "DynamicFactory.h"
#include "processor/Processor.h"
#include <opencv2/opencv.hpp>
#include "common.h"

namespace whale {
namespace vision {

class DetectorProcessor : public Processor, DynamicCreator<DetectorProcessor> {
 public:
    DetectorProcessor() : Processor(WHALE_PROCESSOR_DETECT) {}
    virtual ~DetectorProcessor() {}
    virtual void run();
    virtual void init();

 private:
    void labelBox(cv::Mat& rgbImage,
                  std::vector<whale::core::Box>& det_results);
    cv::Mat template_;
    cv::Mat template2_;
    std::vector<cv::Mat> circles_;
    int frame_idx_ = -1;
    int64_t last_hasface_time_ = 0;

 private:
    std::map<std::string, QueuePtr> next_processor_unit_;
    int64_t last_image_monitor_time_;
    int64_t last_model_metric_monitor_time_;
	int64_t image_monitor_interval;

    bool whale_door_ = false;
};
}  // namespace vision
}  // namespace whale