
#include "init.h"

#include <gflags/gflags.h>
#include <math.h>
#include <openssl/md5.h>

#include <fstream>
#include <thread>
#include <json/json.h>

#include "ServiceMetricManager.h"
#include "SysPublicTool.h"
#include "common.h"
#include "interface.h"
#include "mgr/ConfigureManager.h"
#include "network/HttpClient.h"
#include "processor/Processor.h"
#include "util.h"

//"NOTE: THE CODE'S VERSION"
#define ALGO_VERSION "8.0.4.1"

namespace whale {
namespace vision {

enum class WhaleDeviceType {
    kAICamera,
    kNVR,
    kHKCamera,
};

int gen_company_sn(const char *src, int len, WhaleDeviceType type, int channel,
                   char *dst_sn) {
    if (!src || !dst_sn || len < 19) {
        return -1;
    }

    if (type == WhaleDeviceType::kAICamera) {
        memcpy(dst_sn, "WKA2", 4);
    } else if (type == WhaleDeviceType::kNVR) {
        memcpy(dst_sn, "WVR2", 4);
    } else if (type == WhaleDeviceType::kHKCamera) {
        memcpy(dst_sn, "WKL0", 4);
    } else {
        memcpy(dst_sn, src, 4);
    }

    memcpy(dst_sn + 4, src + 4, 11);

    if (type == WhaleDeviceType::kHKCamera) {
        char lastbuf[4] = {0};
        sprintf(lastbuf, "HK%02X", channel);
        memcpy(dst_sn + 11, lastbuf, 4);
        printf("lastbuf=%s dst_sn=%s \n", lastbuf, dst_sn);
    }

    char *src_sn = dst_sn;
    char *p = src_sn;

    int total = 0;
    int i = 0;
    while (i++ < len - 4) {
        total += *p++;
    }

    char ch = total & 0xFF;
    char ch_buf[2] = {0};
    sprintf(ch_buf, "%02X", ch);

    char md5_sn[19];
    memcpy(md5_sn, src_sn, len - 4);
    memcpy(md5_sn + len - 4, ch_buf, 2);

    unsigned char md5_out[16];
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, md5_sn, len - 2);
    MD5_Final(md5_out, &ctx);
    sprintf(dst_sn + 15, "%02X%02X", md5_out[14], md5_out[15]);
    return 0;
}

void EnvironmentInit::init() {
    gt = GetGlobalVariable();

    initServiceVariable();
    initModelPlugin();

    initMetric();
}

void EnvironmentInit::uploadVersion() {

};

void EnvironmentInit::initMetric() {

}

void EnvironmentInit::initOtherComponent() {
}

void EnvironmentInit::initServiceVariable() {
    // init version
    gt->sys_path = ConfigureManager::instance()->getAsString(WHALE_SYS_PATH);
    std::string version_file = gt->sys_path + "/version";
    gt->sys_version = SysPublicTool::loadConfigItem(version_file);

    // init system sn
    initDeviceSn();

    // init group information of the device
    initDeviceGroupInfo();

    // init code version
    gt->algo_version = ALGO_VERSION;

    // todo other sys env
    // init whale face mqtt topic
    // init pad relate configure item.
    // read configure item from local file.
    std::string sys_path =
        ConfigureManager::instance()->getAsString(WHALE_SYS_PATH);

    gt->face_pad_topic = SysPublicTool::loadConfigItem(sys_path + "/pad_topic");

    gt->pad_context =
        std::stoi(SysPublicTool::loadConfigItem(sys_path + "/pad_context"));
}

std::vector<std::string> EnvironmentInit::readCameraSnFromFile(
    std::string &file) {
    std::vector<std::string> sn_vec;
    std::ifstream fCameraSnRead(file);

    if (!fCameraSnRead) {
        fCameraSnRead.close();
        LOG(ERROR) << "NO DEVICE SN";
        LOG(ERROR) << "PATH: " << file;
        LOG(ERROR) << " please calture camera sn.";
        // log metric
        return sn_vec;
    } else {
        while (!fCameraSnRead.eof()) {
            std::string lineTxt;
            fCameraSnRead >> lineTxt;
            if (!lineTxt.compare(0, 9, "camera_sn")) {
                std::string sn = lineTxt.substr(10, lineTxt.length() - 9);
                sn_vec.emplace_back(sn);
            }
        }
    }
    fCameraSnRead.close();
    return sn_vec;
}

void EnvironmentInit::initDeviceSn() {
#ifndef __PLATFORM_HISI__
    // todo add edge db lib to store the device group info
    std::string sys_path =
        ConfigureManager::instance()->getAsString(WHALE_SYS_PATH);
    std::string clientSnFile = sys_path + "/sys_sn";
    std::ifstream fClientSnRead(clientSnFile);
    if (!fClientSnRead) {
        gt->client_sn = "default";
    } else {
        while (!fClientSnRead.eof()) {
            std::string lineTxt;
            fClientSnRead >> lineTxt;
            if (!lineTxt.compare(0, 9, "client_sn")) {
                gt->client_sn = lineTxt.substr(10, lineTxt.length() - 9);
            }
        }
    }
    fClientSnRead.close();

    std::string cameraSnFile = sys_path + "/camera_sn";
    std::vector<std::string> camera_sn_vec = readCameraSnFromFile(cameraSnFile);
    if (camera_sn_vec.size() > 0) gt->camera_sns = camera_sn_vec[0];

    std::vector<std::string> device_name_vec =
        ConfigureManager::instance()->getAsVectorString("video_input");
    for (size_t i = 0; i < device_name_vec.size(); ++i) {
        if (camera_sn_vec.size() >= i + 1) {
            LOG(INFO) << device_name_vec[i] << " <-rtsp/usb, : sn->"
                      << camera_sn_vec[i];
            gt->video_inputs = device_name_vec[i];
            // ServiceVariableManager::Add(device_name_vec[i],
            // camera_sn_vec[i]);
        }
    }
#else
    gt->client_sn = SysPublicTool::loadConfigItem("/mnt/whale/sn");
    std::this_thread::sleep_for(std::chrono::seconds(5));
    char camera_sn[20] = {0};
    gen_company_sn(gt->client_sn.c_str(), gt->client_sn.length(),
                   WhaleDeviceType::kAICamera, 0, camera_sn);
    LOG(INFO) << "gen camera sn: " << camera_sn;
    gt->camera_sns = camera_sn;
#endif

    // set default service variable,
    gt->group_id = gt->company_id = gt->shop_id = gt->client_sn;
}

void EnvironmentInit::initDeviceGroupInfo() {
    // get device group and  company info
    std::string deviceGroupFile =
        ConfigureManager::instance()->getAsString(WHALE_SYS_PATH) + "/" +
        WHALE_DEVICE_GROUP;
    gt->group_name =
        SysPublicTool::loadConfigItem(deviceGroupFile, "default_device_group");
    LOG(INFO) << "get group_name from file: " << gt->group_name;

//    std::thread t([&]() {
//        bool whale_door = ConfigureManager::instance()->getAsInt("whale_door");
//        if (gt->client_sn == "default") return;
//
//        int interval_sec = 600;
//        bool isprod = !(GetGlobalVariable()->isDevEnv);
//        while (isprod) {
//            std::string response;
//            auto re = HttpGet((WHALE_GROUPINFO_URL + gt->client_sn).c_str(), "",
//                              response);
//
//            LOG(INFO) << "HttpRequestClient::getGroupInfo, rec_code: " << re;
//            if (re) {
//                Json::Value value;
//                Json::CharReaderBuilder jsoriReader;
//                std::unique_ptr<Json::CharReader> const oriReader(jsoriReader.newCharReader());
//                std::string err;
//                oriReader->parse(response.c_str(), response.c_str()+response.size(), &value, &err);
//                if (value["errno"] == 10000) {
//                    std::string company_id =
//                        value["data"]["company_id"].asString();
//                    std::string group_id = value["data"]["group_id"].asString();
//                    std::string group_name =
//                        value["data"]["group_name"].asString();
//                    std::string shop_id = value["data"]["shop_id"].asString();
//
//                    LOG(INFO) << "Dispatch whale door: " << whale_door;
//                    if (whale_door) {
//                        group_id = value["data"]["company_id"].asString();
//                        // todo, modify redis-store key-group info ,'
//                        // because whale door ,redis-store older group info, for
//                        // temp door, todo
//                        group_id = "8f75b476-deda-45f4-a1fb-27e98c0a1a5e";
//                    }
//
//                    gt->group_id = group_id;
//                    gt->company_id = company_id;
//                    gt->shop_id = shop_id;
//                    gt->group_name = group_name;
//
//                    LOG(INFO) << "saved group_name_: " << group_name;
//                    std::string deviceGroupFile =
//                        ConfigureManager::instance()->getAsString(
//                            WHALE_SYS_PATH) +
//                        "/" + WHALE_DEVICE_GROUP;
//                    SysPublicTool::writeConfigItem(deviceGroupFile, group_name);
//
//                    LOG(INFO) << "GET GROUP_ID OK. group_id: " << group_id
//                              << " shop_id: " << shop_id
//                              << " company_id: " << company_id
//                              << " group_name: " << group_name;
//                    break;
//                }
//            }
//            std::this_thread::sleep_for(std::chrono::seconds(interval_sec));
//        }
//    });
//    t.detach();
}

void EnvironmentInit::initModelPlugin() {
    std::string sys_path =
        ConfigureManager::instance()->getAsString("sys_path");
    std::string model_path = sys_path + "models";

    whale::core::sdk::init(
        model_path,
        Json::FastWriter().write(ConfigureManager::instance()->getAsAarry("modules")));
}

}  // namespace vision
}  // namespace whale
