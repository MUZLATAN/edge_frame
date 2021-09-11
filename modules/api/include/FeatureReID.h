#pragma once

#include "inference.h"

namespace whale {
namespace core {
class FeatureReID : public ProcessFactory {
 public:
    FeatureReID(std::string model_name = "FeatureReID")
        : ProcessFactory(model_name) {
        setShape(128, 64);
        output_tensor_names_ = std::vector<std::string>({"batch_norm_blob2"});
    }
    ~FeatureReID() {
        // printf("FeatureReID deconstruct.\n");
    }

    void preProcess(const cv::Mat &image) {
        cv::Mat dst_image =
            cv::Mat::zeros(input_height_, input_width_, CV_8UC3);
        cv::resize(image, dst_image, dst_image.size());
        float *data = pre_data_.data();
        for (int i = 0; i < input_height_; i++) {
            uchar *inData = dst_image.ptr<uchar>(i);
            for (int j = 0; j < input_width_; j++) {
                *(data + i * input_width_ + j) =
                    ((float)(*inData++) / 255.0f - 0.485f) / 0.229f;

                *(data + input_height_ * input_width_ + i * input_width_ + j) =
                    // ((float)(*inData++) / 255.0f - 0.456f) / 0.224f;
                    ((float)(*inData++) / 255.0f - 0.485f) / 0.229f;

                *(data + 2 * input_height_ * input_width_ + i * input_width_ +
                  j) =
                    // ((float)(*inData++) / 255.0f - 0.406f) / 0.225f;
                    ((float)(*inData++) / 255.0f - 0.485f) / 0.229f;
            }
        }
    }

    void postProcess(std::unordered_map<std::string, float *> &tensor_data_map,
                     const boost::any &any) {
        float *outdata = tensor_data_map["batch_norm_blob2"];
        std::vector<float> *feature =
            boost::any_cast<std::vector<float> *>(any);
        postProcess(feature, outdata);
    }

    void postProcess(std::vector<float> *feature, float *outdata) {
        feature->insert(feature->begin(), outdata, outdata + 512);
    }
};
}  // namespace core
}  // namespace whale
