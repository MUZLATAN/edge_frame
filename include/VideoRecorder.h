#pragma once
#include <glog/logging.h>

#include <fstream>
#include <memory>
#include <mutex>
#include <opencv2/videoio.hpp>
#include <string>
#include <vector>

namespace whale {
namespace vision {

void checkRecordDir(void);

void removeUndoneFile(void);

std::string getCompletionStamp(void);

void addCompletionStamp(const std::string &filepath);

std::string getRecordDir(void);

inline std::string readFile(std::string fileName) {
    std::ifstream t(fileName);
    if (!t.is_open()) {
        LOG(ERROR) << "cannot open file: " << fileName;
        return "";
    }
    std::stringstream buffer;
    buffer << t.rdbuf();
    t.close();
    return buffer.str();
};

class UserInfoRecorder {
 private:
    std::string infoFileName_ = "";

 public:
    std::string getInfoFileName(void);
    void writeInfo(const std::string &filename, const char *info,
                   const int size);
};

class RGBVideoRecorder {
 private:
    std::string videoFileName_ = "";
    int videoFrameRate_ = -1;
    int videoWidth_ = -1;
    int videoHeight_ = -1;
    int64_t recorderFrameNum_ = 0;
    int64_t recordingAllFrameNum_ = 0;
    cv::VideoWriter videoWriter_;
    std::mutex threadMt_;

 public:
    RGBVideoRecorder();
    ~RGBVideoRecorder();

    std::string getVideoFileName(void);
    void set(std::string videoFileName, int Fps, int width, int height,
             float videolength);  // video length指的是录制时长，单位为分钟
    void set(std::string videoFileName);
    bool write(const cv::Mat &image);
    void forceStop(void);
    bool isRecordOver(void);
};

}  // namespace vision
}  // namespace whale
