#pragma once
#include <tuple>
#include <vector>

#include "inference.h"

namespace whale {
namespace core {

enum class NMSTYPE {
    hard_nms = 1,
    blending_nms = 2,
};

enum class EModelID {
    e_ped_v3 = 1,
    e_monitor_v1,
    e_capture_v2,
    e_huishoubao_v2,
    e_vehicle_v1,
};

extern std::map<std::string, std::pair<EModelID, std::vector<std::string>>>
    DetectorCommonVmap;

class Detect : public ProcessFactory {
 public:
    Detect(std::string model_name = "Detect") : ProcessFactory(model_name){};
    ~Detect(){};
    void preProcess(const cv::Mat &image);
    void run(const cv::Mat &image, std::vector<whale::core::Box> &output_boxes,
             std::vector<whale::core::Box> &proposal_boxes,
             bool proposal_flag = false);
    void postProcess(std::unordered_map<std::string, float *> &tensor_data_map,
                     std::vector<whale::core::Box> &boxes);
    void postProcess(float *pOutputData_box, float *pOutputData_score,
                     std::vector<whale::core::Box> &output_boxes);
    void setJson(const std::string &det_param_json);

 private:
    void generateBBox(
        std::vector<std::vector<whale::core::Box>> &proposal_boxes,
        const float *out_boxes, const float *out_score, float score_threshold);

    void nms(std::vector<std::vector<whale::core::Box>> &proposal_boxes,
             std::vector<whale::core::Box> &output_boxes, NMSTYPE type);
    void input_transform(cv::Mat &image, float *input_data, int img_h,
                         int img_w);

 protected:
    float score_threshold;
    float iou_threshold;
    int num_of_class;
    int num_anchors;
    int image_h = 0;
    int image_w = 0;
    std::vector<std::vector<std::vector<float>>> min_boxes;
    std::vector<std::vector<float>> featuremap_size;
    float center_variance;
    float size_variance;
    float means[3] = {128, 128, 128};
    float scale[3] = {1.0, 1.0, 1.0};
    int num_featuremap;

    std::string model_box_l_name;
    std::string model_score_l_name;

 private:
    std::vector<std::vector<float>> priors;
};
}  // namespace core
}  // namespace whale