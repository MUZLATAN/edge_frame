#pragma once

#include <functional>
#include <unordered_map>

#include "inference.h"

namespace whale {
namespace core {

class MobileFacenet : public ProcessFactory {
 public:
    MobileFacenet(std::string model_name = "MobileFacenet")
        : ProcessFactory(model_name) {
        setShape(112, 96);
    }
    ~MobileFacenet() {}

    void preProcess(cv::Mat &img) {
        cv::resize(img, img, cv::Size(input_width_, input_height_), 0, 0);
        unsigned char *src_ptr = (unsigned char *)(img.ptr(0));
        int hw = input_height_ * input_width_;
        float *data = pre_data_.data();
        for (int h = 0; h < input_height_; h++) {
            for (int w = 0; w < input_width_; w++) {
                for (int c = 0; c < 3; c++) {
                    data[c * hw + h * input_width_ + w] =
                        (float)(*src_ptr - 127.5) / 128.0;
                    src_ptr++;
                }
            }
        }
    }

    void postProcess(std::unordered_map<std::string, float *> &tensor_data_map,
                     const boost::any &any) {
        float *result_embedding = boost::any_cast<float *>(any);
        float *outdata = tensor_data_map["view1"];
        postProcess(result_embedding, outdata);
    }

    void postProcess(float *result_embedding, float *outdata) {
        memcpy(result_embedding, outdata, sizeof(float) * 128);
        for (int i = 0; i < 128; ++i) {
            LOG(INFO) << i << " " << result_embedding[i];
        }
    }
};
}  // namespace core
}  // namespace whale