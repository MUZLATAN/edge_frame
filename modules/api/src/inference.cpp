

#include "inference.h"

namespace whale {
namespace core {

ProcessFactory::ProcessFactory(){};

ProcessFactory::~ProcessFactory(){};

ProcessFactory::ProcessFactory(std::string model_name)
    : model_name_(model_name){};

void ProcessFactory::setShape(int input_height, int input_width) {
    input_height_ = input_height;
    input_width_ = input_width;
    pre_data_.resize(input_height_ * input_width_ * 3);
};

void ProcessFactory::getShape(int &input_height, int &input_width) {
    input_height = input_height_;
    input_width = input_width_;
};

float *ProcessFactory::getPreData() { return pre_data_.data(); };

std::string ProcessFactory::getModelName() { return model_name_; };

const std::vector<std::string> &ProcessFactory::getOutTensorName(void) {
    return output_tensor_names_;
};

}  // namespace core
}  // namespace whale