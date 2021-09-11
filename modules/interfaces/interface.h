#pragma once

#include <memory>
#include <string>
#include <vector>

#include "AlgoData.h"
namespace whale {
namespace core {
namespace sdk {

void init(const std::string &model_path, std::string json);

void detect(std::shared_ptr<whale::core::BaseFrame> frame,
            std::vector<whale::core::Box> &boxes);
void detect(const cv::Mat &frame, std::vector<whale::core::Box> &boxes);

void reid(const cv::Mat &frame, std::vector<float> &feature);

void attribute(const cv::Mat &frame, whale::core::Attribute &attri);

void score(const cv::Mat &image, const cv::Rect &rect,
           FaceQuality &image_score);

void facefeature(const cv::Mat &image, float *feature);

std::string getversion();

}  // namespace sdk
}  // namespace core
}  // namespace whale