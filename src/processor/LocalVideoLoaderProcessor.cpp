#include <dirent.h>
#include <fnmatch.h>

#include <chrono>
//#include <opencv2/opencv.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "SolutionPipeline.h"
#include "SysPublicTool.h"
#include "mgr/ConfigureManager.h"
#include "mgr/QueueManager.h"
#include "processor/DataLoaderProcessor.h"

#define VIDEOFORMAT ".mp4"

namespace whale {
namespace vision {
void LocalVideoLoaderProcessor::init() {
    QueueManager::SafeGet(WHALE_PROCESSOR_DETECT, output_queue_);
    if (local_video_file_.find(VIDEOFORMAT) !=
        std::string::npos)  // only read one file
        localVideoList_.push_back(local_video_file_);
    else
        getLocalRestVideo(
            local_video_file_);  // search all files that under path

    if (loadVideo() != 0) LOG(FATAL) << "ERROR: CANNOT FIND ANLY VIDEO !!!";
}

void LocalVideoLoaderProcessor::run() {
    while (true) {
        if (gt->sys_quit) {
            return;
        }

        auto frame = std::make_shared<whale::core::WhaleFrame>();
        capture_->read(frame->cvImage);
        if (frame->cvImage.empty()) {
            LOG(INFO) << local_video_file_ << " read complete.";
            if (loadVideo() == 0)
                continue;
            else {
                std::this_thread::sleep_for(std::chrono::seconds(10));
                gt->sys_quit = true;  // system quit
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
                LOG(INFO) << processor_name_ << " exit .......";
                return;
            }
        }

        frame_id_++;
        frame->setCurrentTime();
//        frame->camera_time = frame_id_;
        frame->camera_sn = gt->camera_sns;
        frame->frame_id = frame_id_;

        output_queue_->Push(frame, false);
        is_ready_ = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(66));
    }
}

void LocalVideoLoaderProcessor::getLocalRestVideo(std::string path) {
    SysPublicTool::getFiles(path, localVideoList_, VIDEOFORMAT);
    std::string csvPath = path + "/csvs";
    local_csv_dir_ = csvPath;
    LOG(INFO) << " local csv dir:" << local_csv_dir_;
    SysPublicTool::_mkdir(csvPath.c_str());
}

int LocalVideoLoaderProcessor::loadVideo() {
    if (!localVideoList_.empty()) {
        //列表不空就继续读取
        local_video_file_ = localVideoList_.back();
        localVideoList_.pop_back();
    } else
        return -1;

    if (capture_) {
        capture_->release();
        capture_->open(local_video_file_);
    } else
        capture_ = std::make_unique<cv::VideoCapture>(local_video_file_);

    CHECK(capture_->isOpened()) << "ERROR: video name is invalid !!!";

    LOG(INFO) << "load local file:" << local_video_file_
              << ", Number of videos left is: " << localVideoList_.size();

    capture_->set(cv::CAP_PROP_BUFFERSIZE, 3);
    capture_->set(cv::CAP_PROP_FPS, 15);

    std::vector<std::string> nameList =
        SysPublicTool::split(local_video_file_, "/");

    std::string lastName = nameList.back();

    return 0;
}
}  // namespace vision
}  // namespace whale