#pragma once

#include <map>
#include <opencv2/opencv.hpp>

#include "DynamicFactory.h"
#include "Processor.h"
#include "TrackObjectInterface.h"
#include "tracker.h"

namespace whale {
namespace vision {
;
class MonitorProcessor : public Processor, DynamicCreator<MonitorProcessor> {
 public:
    MonitorProcessor() : Processor(WHALE_PROCESSOR_MONITOR) {}
    virtual ~MonitorProcessor() {}
    virtual void run();

    virtual void init();

 private:
    void handle(std::shared_ptr<WhaleData> message);
    void CvtDetToPed(vector<Box> &det_boxes, vector<PersonObject> &ped_det,
                     unsigned long long timestamp, unsigned long long frame_id);
    void monitor(const std::string &camera_sn, int64_t camera_time,
                 cv::Mat frame, vector<PersonObject> &ped_tracked,
                 vector<PersonObject> &ped_leave);

    void visualize(cv::Mat frame, cv::Mat &image_show,
                   vector<PersonObject> &ped_tracked);

 private:
    std::vector<int> x_;
    std::vector<int> y_;
    std::vector<int> id_;
    bool need_visualize;
    std::shared_ptr<Tracker> tracker_;
    int cur_interval_cnt_;
    int monitor_interval;
    std::map<std::string, int> cls_id_map;
};
}  // namespace vision
}  // namespace whale