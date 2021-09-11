#include "VideoRecorder.h"

#include <dirent.h>
#include <glog/logging.h>
#include <opencv2/videoio/videoio_c.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>

#include "SysPublicTool.h"

// #include <experimental/filesystem>

#include <stdio.h>

#include <boost/filesystem.hpp>

using namespace std;
using namespace boost::filesystem;

#define _VIDEORECORDSAVEDIR_ "/home/nvidia/FLOW/bin/RGBVideoRecord/"
#define _VIDEORECORDEOVER_ ".over"

namespace whale {
namespace vision {

std::string getCompletionStamp(void) { return _VIDEORECORDEOVER_; };

void addCompletionStamp(const string &filename) {
    auto newname = filename + _VIDEORECORDEOVER_;
    int result = rename(filename.c_str(), newname.c_str());
    if (result == 0)
        LOG(INFO) << "File successfully renamed: " << filename;
    else
        LOG(FATAL) << "Error renaming file: " << filename << ", result is "
                   << result;
};

void removeUndoneFile() {
    path p(_VIDEORECORDSAVEDIR_);
    for (auto i = directory_iterator(p); i != directory_iterator(); i++) {
        if (!is_directory(i->path()))  // we eliminate directories
        {
            LOG(INFO) << "NOTE: file full name "
                      << i->path().filename().string();
            auto filename =
                _VIDEORECORDSAVEDIR_ + i->path().filename().string();
            std::string format = filename.substr(
                filename.size() - sizeof(_VIDEORECORDEOVER_) + 1,
                sizeof(_VIDEORECORDEOVER_) - 1);
            LOG(INFO) << "NOTE: file format is: " << format;

            if (format != _VIDEORECORDEOVER_) {
                if (remove((filename).c_str()) != 0) {
                    LOG(ERROR) << "cannot remove file:" << filename;
                    perror("remove");
                }
                LOG(INFO) << "File successfully deleted:" << filename;
            }
        } else
            continue;
    }
};

void checkRecordDir() {
    DIR *dir = opendir(_VIDEORECORDSAVEDIR_);
    if (dir == NULL)
        SysPublicTool::_mkdir(_VIDEORECORDSAVEDIR_);
    else
        closedir(dir);
};

std::string getRecordDir(void) { return _VIDEORECORDSAVEDIR_; };

std::string UserInfoRecorder::getInfoFileName(void) { return infoFileName_; };

void UserInfoRecorder::writeInfo(const std::string &filename, const char *info,
                                 const int size) {
    infoFileName_ = filename;
    std::ofstream fout(infoFileName_, std::ios::binary);
    fout.write(info, size);
    fout.close();
};

RGBVideoRecorder::RGBVideoRecorder() {}

RGBVideoRecorder::~RGBVideoRecorder() {}

std::string RGBVideoRecorder::getVideoFileName() { return videoFileName_; }

void RGBVideoRecorder::set(std::string videoFileName, int Fps, int width,
                           int height, float videolength) {
    forceStop();
    videoFrameRate_ = Fps;
    recordingAllFrameNum_ = videolength * videoFrameRate_ * 60;
    videoWidth_ = width;
    videoHeight_ = height;
    recorderFrameNum_ = 0;
    videoFileName_ = videoFileName;
    // cur_file_name_ = root_dir_path_ + client_sn_ + "_" +
    // 								 SysPublicTool::getCurrentTimeString()
    // +
    // ".mp4";

    videoWriter_.open(videoFileName_, 0x21 /*CV_FOURCC('h', '2', '6', '4')*/,
                      videoFrameRate_, cv::Size(videoWidth_, videoHeight_));

    LOG(INFO) << "NOTE: Video recording setup complete ";
}

void RGBVideoRecorder::set(std::string videoFileNam) {
}

bool RGBVideoRecorder::write(const cv::Mat &image) {
    if (image.empty()) return false;
    std::lock_guard<std::mutex> lock(threadMt_);
    if (isRecordOver()) return false;

    // TODO: jinpengli: 视频抽帧
    cv::Mat tmp;
    cv::resize(image, tmp, cv::Size(videoWidth_, videoHeight_));
    videoWriter_.write(tmp);
    recorderFrameNum_++;
    return true;
}

void RGBVideoRecorder::forceStop() {
    std::lock_guard<std::mutex> lock(threadMt_);
    if (videoWriter_.isOpened()) {
        videoWriter_.release();
        LOG(INFO) << "NOTE: Video recording has stopped, file name is " +
                         videoFileName_;
    }
}

bool RGBVideoRecorder::isRecordOver() {
    return recorderFrameNum_ >= recordingAllFrameNum_;
}
}  // namespace vision
}  // namespace whale