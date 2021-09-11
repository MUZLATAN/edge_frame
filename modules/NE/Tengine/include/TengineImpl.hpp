
#pragma once

#include <glog/logging.h>
#include <tengine/tengine_c_api.h>
#include <tengine/tengine_operations.h>

#include "Detect.h"
#include "FaceAttribute.h"
#include "FaceDetectWithMask.h"
#include "FaceFeature.h"
#include "FaceScore.h"
#include "FeatureReID.h"
#include "inference.h"

namespace whale {
namespace core {

template <typename T>
class TengineInf {
 public:
    TengineInf(){};
    ~TengineInf() {
        if (input_tensor_) release_graph_tensor(input_tensor_);
        if (output_tensor_) release_graph_tensor(output_tensor_);
        if (graph_) destroy_graph(graph_);

        input_tensor_ = nullptr;
        output_tensor_ = nullptr;
        graph_ = nullptr;
    };

    void print(const char* s) { LOG(INFO) << s; };

    template <typename... Type>
    void print(const char* s, Type&&... value) {
        print(s);
        print(std::forward<Type>(value)...);
    };

    template <typename... Type>
    void init(const std::string& json, std::string model_type,
              Type&&... value) {
        processsF_.setJson(json);
        init(model_type, std::forward<Type>(value)...);
    };

    template <typename... Type>
    void init(std::string model_type, Type&&... value) {
        LOG(INFO) << "NOTE: MODEL NAME IS " << processsF_.getModelName()
                  << "  , create version: " << model_type;
        graph_ = create_graph(nullptr, model_type.c_str(),
                              std::forward<Type>(value)...);

        if (graph_ == nullptr) {
            LOG(ERROR) << "Create graph0 failed! errno: " << get_tengine_errno()
                       << " , args is ";
            print(std::forward<Type>(value)...);
            exit(-1);
        }

        // TODO: it maybe error
        int ret = prerun_graph(graph_);
        if (ret != 0) {
            LOG(INFO) << "Prerun graph failed, errno: " << get_tengine_errno()
                      << "\n";
            return;
        }

        LOG(INFO) << "Create graph0 ok. args is ";
        print(std::forward<Type>(value)...);
        LOG(INFO) << "inferor: " << this << ", model_type: " << model_type
                  << ", graph: " << graph_;
    };

    template <typename... Type>
    void run(std::shared_ptr<whale::core::BaseFrame> frame, Type&&... value) {
        run(frame->cvImage, std::forward<Type>(value)...);
    };

    template <typename... Type>
    void run(const cv::Mat& image, Type&&... value) {
        if (image.empty()) {
            LOG(ERROR) << "ERROR: image is empyty !!!";
            return;
        }

        processsF_.preProcess(image);
        std::unordered_map<std::string, float*> result;
        int input_height = 0;
        int input_width = 0;
        processsF_.getShape(input_height, input_width);
        // core impl
        impl(processsF_.getPreData(), batch_, input_height, input_width,
             result);
        // core impl
        processsF_.postProcess(result, std::forward<Type>(value)...);
    };

 private:
    void impl(float* input_data, int batch, int height, int width,
              std::unordered_map<std::string, float*>& results) {
        input_tensor_ = get_graph_input_tensor(graph_, 0, 0);
        int dims[] = {batch, 3, height, width};
        int data_size = batch * 3 * height * width * sizeof(float);
        set_tensor_shape(input_tensor_, dims, 4);
        if (set_tensor_buffer(input_tensor_, input_data, data_size) < 0) {
            std::printf("Set buffer for tensor failed\n");
            return;
        }

        int ret = run_graph(graph_, 1);
        if (ret < 0) {
            LOG(INFO) << "tengine run_graph failed, errno: "
                      << get_tengine_errno() << "\n";
            return;
        }
        // dump_graph(graph_);

        // get output tensor
        auto output_tensor_names_ = processsF_.getOutTensorName();
        if (output_tensor_names_.size() > 0) {
            for (int i = 0; i < output_tensor_names_.size(); ++i) {
                void* tensor =
                    get_graph_tensor(graph_, output_tensor_names_[i].c_str());
                float* tensor_data = (float*)get_tensor_buffer(tensor);

                // int dim_cls[4] = {0};
                // get_tensor_shape(tensor, dim_cls, 4);

                results[output_tensor_names_[i]] = tensor_data;
            }
        } else {
            output_tensor_ = get_graph_output_tensor(graph_, 0, 0);
            float* tensor_data = (float*)get_tensor_buffer(output_tensor_);
            results["default"] = tensor_data;
        }
    };

 private:
    T processsF_;
    int batch_ = 1;

    std::vector<float> input_data_;
    void* input_tensor_ = nullptr;
    void* output_tensor_ = nullptr;
    void* graph_ = nullptr;
};

typedef TengineInf<Detect> DetectImpl;
typedef TengineInf<FaceAttribute> FaceAttributeImpl;
typedef TengineInf<FeatureReID> FeatureReIDImpl;

typedef TengineInf<FaceScore> FaceeScoreImpl;
typedef TengineInf<FaceScoreOcc> FaceOccludeImpl;
typedef TengineInf<FaceScorePose> FacePoseImpl;

#ifdef __BUILD_FACEDOOR__
typedef TengineInf<FaceFeature> FaceFeatureImpl;
typedef TengineInf<FaceDetectWithMask> FaceDetectWithMaskImpl;
#endif

}  // namespace core
}  // namespace whale