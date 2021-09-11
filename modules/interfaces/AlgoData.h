#pragma once
#include <chrono>
#include <memory>
#include <opencv2/opencv.hpp>

namespace whale {

namespace core {

struct landmark {
    float x[5];
    float y[5];
};

typedef struct Box {
    cv::Rect rect;
    float x0;
    float y0;
    float x1;
    float y1;
    float confidence;
    float score;
    int id;
    int class_idx;
    float landmark_score;
    struct landmark points;
    bool with_mask;

} Box;

typedef struct Attribute {
    int age;
    int gender;
    int mask;
} Attribute;

enum CategoryId { cls_face = 2, cls_head = 3, cls_ped = 1 };

typedef struct FaceQuality {
    float yaw;
    float pitch;
    float roll;
    float occlude;
    float blur;
    float rscore;
    float score;
    FaceQuality() {
        yaw = pitch = roll = -1000;
        occlude = blur = rscore = score = -1.0;
    }
} FaceQuality;

class BaseFrame {
 public:
    BaseFrame() { setCurrentTime(); };
    BaseFrame(const BaseFrame& f)
        : camera_time(f.camera_time),
          cvImage(f.cvImage),
          camera_sn(f.camera_sn),
          frame_id(f.frame_id){};
    int64_t getCurrentTime() { return camera_time; };
    virtual cv::Mat& getcvMat(void) { return cvImage; };
    // heigth
    virtual int row(void) { return cvImage.rows; };
    // width
    virtual int col(void) { return cvImage.cols; };

    void setCurrentTime() {
        camera_time = std::chrono::system_clock::now().time_since_epoch() /
                      std::chrono::milliseconds(1);
    };

    // NOTE: crop func may change the rect !
    virtual std::shared_ptr<BaseFrame> crop(cv::Rect rect) {
        std::shared_ptr<BaseFrame> frame_ = std::make_shared<BaseFrame>(*this);
        frame_->cvImage = cvImage(rect);
        return frame_;
    };

    virtual std::shared_ptr<BaseFrame> cropAndResize(cv::Rect rect,
                                                     cv::Size dsize) {
        std::shared_ptr<BaseFrame> frame_ = std::make_shared<BaseFrame>(*this);
        cv::Mat tmp;
        tmp = cvImage(rect);
        cv::resize(tmp, frame_->cvImage, dsize);
        return frame_;
    };

 public:
    int64_t camera_time = 0;
    cv::Mat cvImage;
    std::string camera_sn;  // TODO: is this variable usable?F
    unsigned long long frame_id = 0;
};

}  // namespace core
}  // namespace whale