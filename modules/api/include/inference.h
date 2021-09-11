
#pragma once

#include <glog/logging.h>

#include <boost/any.hpp>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>

#include "AlgoData.h"

#define TIME_FUNC(ME)                                                \
    if (true) {                                                      \
        auto start = std::chrono::steady_clock::now();               \
        ME;                                                          \
        auto end = std::chrono::steady_clock::now();                 \
        std::chrono::duration<double, std::milli> du = end - start;  \
        LOG(INFO) << #ME << "time usage is " << du.count() << " ms"; \
    }

namespace whale {
namespace core {

template <typename T>
class Impl {
 public:
    Impl(){};
    ~Impl(){};
    template <typename... Type>
    void init(Type &&... value) {
        // std::initializer_list<std::string>{(addPath(value), 0)...};
        // modelPath_ = std::vector<std::string>({value...});

        // LOG(INFO) << "NOTE: init impl: " ;
        inf_.init(std::forward<Type>(value)...);
    };

    template <typename... Type>
    void run(Type &&... value) {
        inf_.run(std::forward<Type>(value)...);
    };
    // template <typename... Type>
    // void run(const cv::Mat &image, Type &&... value) {
    // 	if (image.empty()) {
    // 		std::printf("detect image empty\n.");
    // 		return;
    // 	}

    // 	preProcess(image);
    // 	std::unordered_map<std::string, float *> result;
    // 	// core impl
    // 	inferor_impl_->impl(data_.data(), 1, input_height_, input_width_,
    // 											result);
    // 	// core impl
    // 	postProcess(result, any);
    // };
    // void setOutputTensorNames(std::vector<std::string> &out_names) ;

    //  private:
    // 	void addPath(const std::string &path) { modelPath_.push_back(path); };

 private:
    T inf_;
    // std::vector<std::string> modelPath_;	// storage for nn model
};

class ProcessFactory {
 public:
    ProcessFactory();
    ProcessFactory(std::string model_name);
    ~ProcessFactory();
    void preProcess();
    void postProcess();
    void setShape(int input_height, int input_width);
    void getShape(int &input_height, int &input_width);
    float *getPreData(void);
    std::string getModelName(void);
    const std::vector<std::string> &getOutTensorName(void);
    void setJson(const std::string &config){};

 protected:
    std::vector<std::string> output_tensor_names_;
    int input_height_ = -1;
    int input_width_ = -1;
    std::string model_name_;        // model name
    std::vector<float> pre_data_;   // storage for pre_data_
    std::vector<float> post_data_;  // storage for post_data_
};

}  // namespace core
}  // namespace whale