#include <unistd.h>

#include <chrono>
#include <cmath>
#include <iostream>
#include <map>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

#include "inference.h"

namespace whale {
namespace core {

class FaceDetectWithMask : public ProcessFactory {
 public:
    FaceDetectWithMask(float height = 224, float width = 224,
                       bool only_top1 = false, bool soft_nms = false);
    ~FaceDetectWithMask();

    void preProcess(const cv::Mat& image);
    void postProcess(std::unordered_map<std::string, float*>& tensor_data_map,
                     std::vector<whale::core::Box>& info);
    void postProcess(float* box_point, float* cls_point, float* land_point,
                     float* mask_point, std::vector<whale::core::Box>& info);

 private:
    void prepare_generate_anchor();
    void generate_anchors(std::vector<std::vector<float>>& min_sizes,
                          std::vector<float>& steps);
    void generator_anchors();
    float clip_label_value(float x, float high);

 private:
    bool only_top1_;
    std::vector<float> bboxes_;
    std::vector<std::vector<float>> priori_anchors_;
    int dim_box[4] = {0};
    int dim_cls[4] = {0};
    int dim_land[4] = {0};
    int dim_mask[4] = {0};
    float ori_height_;
    float ori_width_;
    float score_threshold_ = 0.5;
    float mask_threshold_ = 0.52;
    float nms_threshold_ = 0.6;
    bool soft_nms_;
    float least_percent_of_edge_ = 0.05;
};
}  // namespace core
}  // namespace whale
