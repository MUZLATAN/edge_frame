
#include "app/ServiceDaemon.h"


#include <gflags/gflags.h>
#include <stdlib.h>


#include <boost/lexical_cast.hpp>
#include <chrono>
#include <fstream>
#include <memory>
#include <thread>

#include "SolutionPipeline.h"
#include "SysPublicTool.h"
#include "glog/logging.h"
#include "mgr/ConfigureManager.h"
#include "processor/DynamicFactory.h"

using namespace whale::vision;
using namespace std::chrono;

ServiceDaemon::ServiceDaemon(int argc, char** argv) : version_("") {
    LOG(INFO) << "NOTE: ServiceDaemon Start ";

    auto curTime = SysPublicTool::getCurrentTime();
    lastReadyToFlashConfTime_ = 0;
    nextFlashConfTime_ = 0;

    env.init();
}

void ServiceDaemon::Loop() {
    std::vector<std::string> features =
        ConfigureManager::instance()->getAsVectorString("feature");
LOG(INFO)<<"Build and start global------------------------------";
    SolutionPipeline::BuildAndStartGlobal();
LOG(INFO)<<"build for  global------------------------------";
    // init solutionpipeline
    for (int i = 0; i < features.size(); ++i) {
        LOG(INFO) << i << " " << features[i];
        SolutionPipelinePtr sptr =
            std::make_shared<SolutionPipeline>(features[i]);
        if (sptr) {
            sptr->Build();
            sptr->Start();
            SolutionPipelineManager::SafeAdd(features[i], sptr);
        }
    }
LOG(INFO)<<"BuildAndStartCoreGlobal -------------------------------";
    SolutionPipeline::BuildAndStartCoreGlobal();
LOG(INFO)<<"BuildVideoInput -------------------------------";
    SolutionPipeline::BuildVideoInput();
    SolutionPipeline::StartVideoInput();
    LOG(INFO)<<"BuildVideoInput  end-------------------------------";
    confPeriod_ = 6 * 60 * 60 * 1000;

    int timeClock = 0;
    auto gt = GetGlobalVariable();

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(15));
        if (gt->sys_quit) {
            // todo  wait queue size -> 0
            SolutionPipeline::StopAll();
            std::this_thread::sleep_for(std::chrono::seconds(15));
            exit(0);
        }

        std::this_thread::sleep_for(std::chrono::seconds(15));
    }
}

void ServiceDaemon::updateConf() {
}

ServiceDaemon::~ServiceDaemon() { google::ShutdownGoogleLogging(); }

void ServiceDaemon::updateRecordConf() {
}
