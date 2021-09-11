#include <dirent.h>
#include <fnmatch.h>

#include <chrono>
#include <opencv2/opencv.hpp>
#include <thread>

#include "ServiceMetricManager.h"
#include "mgr/ConfigureManager.h"
#include "mgr/QueueManager.h"
#include "processor/DataLoaderProcessor.h"

namespace whale {
namespace vision {

void RtspLoaderProcessor::init() {
    // todo
    QueueManager::SafeGet(WHALE_PROCESSOR_DETECT, output_queue_);

    capture_ = std::make_unique<cv::VideoCapture>(rtsp_stream_addr_);
    capture_->set(cv::CAP_PROP_BUFFERSIZE, 3);
    capture_->set(cv::CAP_PROP_FPS, 15);

    LOG(INFO) << "rtsp init done";
}

void RtspLoaderProcessor::reopenCamera() {
    while (true) {
        is_ready_ = false;
        if (capture_) {
            capture_->release();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        LOG(INFO) << "******Trying to reconnect rtsp camera now****";

        bool opened = capture_->open(rtsp_stream_addr_);

        if (!capture_->isOpened()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        cv::Mat frame;
        capture_->read(frame);
        if (frame.empty()) {
            LOG(INFO) << "opened: " << frame.cols << " " << frame.rows;
            continue;
        }
        is_ready_ = true;
        return;
    }
}

void RtspLoaderProcessor::run() {
    LOG(INFO) << "rtsp thread run...." << gt->camera_sns;
    int64_t last_heartbeat_time = 0;

    while (true) {
        if (gt->sys_quit) {
            return;
        }

        frame_id_++;

        auto frame = std::make_shared<whale::core::WhaleFrame>();
        capture_->read(frame->cvImage);

        if (frame->cvImage.empty()) {
            reopenCamera();
            continue;
        }
        // TODO: this place need to test mem addr
        frame->cvImage = frame->cvImage.clone();

        frame->setCurrentTime();
        frame->camera_sn = gt->camera_sns;
        frame->frame_id = frame_id_;
        output_queue_->Push(frame, false);

        /*@note in frame count, cv_input_frame_count*/
        ServiceMetricManager::add(MetricAction::kAlgoModelMonitor,
                                  "cv_input_frame_count", 1);

        is_ready_ = true;

        if (frame->camera_time - last_heartbeat_time >
            WHALE_HEARTBEAT_INTERVAL) {
            IOTHeartBeat(gt->camera_sns);
            last_heartbeat_time = frame->camera_time;
        }

        videoRe(frame);
    }
    LOG(INFO) << processor_name_ << " exit .......";
}

}  // namespace vision
}  // namespace whale
