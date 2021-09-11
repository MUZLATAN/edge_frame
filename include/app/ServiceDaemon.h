#pragma once



#include <string>

#include "init.h"

namespace whale {
namespace vision {
class ServiceDaemon {
 public:
    ServiceDaemon(int argc, char** argv);
    ~ServiceDaemon();

    void Loop();
    bool Upgrade();

 private:
    void InitVersion();
    void ReadDeviceSn();

    void OdsVersion();

    void updateConf();  //定时获取配置并设置

    void updateRecordConf();  //定时获取录制视频的配置

 private:
 private:
    std::string flow_home_;
    std::string version_;

    std::string version_file_;

    std::string config_path_;
    std::string config_fullname_;
    int64_t lastReadyToFlashConfTime_;  //上次更新配置时间
    int64_t nextFlashConfTime_;         //计划下次更新配置时间

    int64_t confPeriod_;       //配置有效时间段

    EnvironmentInit env;
};

}  // namespace vision
}  // namespace whale
