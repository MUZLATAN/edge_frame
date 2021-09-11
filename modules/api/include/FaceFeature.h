#pragma once

#include <math.h>

#include <iostream>
#include <unordered_map>

#include "AlgoData.h"
#include "inference.h"

namespace whale {
namespace core {
class FaceFeature : public ProcessFactory {
 public:
    FaceFeature(std::string model_name = "FaceFeature")
        : ProcessFactory(model_name) {
        setShape(112, 112);
        // output_tensor_names_ = std::vector<std::string>({"default"});
    }

    ~FaceFeature() {
        // std::cout << "face feature deconstruct" << std::endl;
    }

    void preProcess(const cv::Mat &image) {
        float *data = pre_data_.data();
        unsigned char *src_ptr = (unsigned char *)(image.ptr(0));
        int hw = input_height_ * input_width_;
        for (int h = 0; h < input_height_; h++) {
            for (int w = 0; w < input_width_; w++) {
                for (int c = 2; c >= 0; --c) {
                    data[c * hw + h * input_width_ + w] =
                        0.0078125 * (float)(*src_ptr - 127.5);
                    ++src_ptr;
                }
            }
        }
    }

    void postProcess(std::unordered_map<std::string, float *> &tensor_data_map,
                     const boost::any &any) {
        // one Box
        float *outdata = tensor_data_map["default"];
        float *feature = boost::any_cast<float *>(any);
        postProcess(feature, outdata);
    };

    void postProcess(float *feature, float *outdata) {
        memcpy(feature, outdata, sizeof(float) * 512);
    };
};
}  // namespace core
}  // namespace whale
