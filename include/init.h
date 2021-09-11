#pragma once
#include <string>
#include <vector>

#include "common.h"

namespace whale {
namespace vision {
class EnvironmentInit {
 public:
    void init();

 private:
    void initServiceVariable();
    std::vector<std::string> readCameraSnFromFile(std::string& file);
    void initDeviceSn();
    void initDeviceGroupInfo();
    void initModelPlugin();
    void initOtherComponent();
    void uploadVersion();
    void initMetric();

 private:
    GlobalVariable* gt = nullptr;
};
}  // namespace vision
}  // namespace whale
