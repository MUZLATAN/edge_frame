#include "event/Event.h"

#include <json/json.h>
#include <glog/logging.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
#include <fstream>

#include "ServiceMetricManager.h"
#include "SysPublicTool.h"
#include "VideoRecorder.h"
#include "interface.h"
#include "network/HttpClient.h"
#include "processor/FlowRpcAsynProcessor.h"

#define TIME_FUNC(ME)                                               \
    if (true) {                                                     \
        auto start = std::chrono::steady_clock::now();              \
        ME;                                                         \
        auto end = std::chrono::steady_clock::now();                \
        std::chrono::duration<double, std::milli> du = end - start; \
        LOG(INFO) << #ME << "time usage ms is " << du.count();      \
    }

namespace whale {
namespace vision {

#ifdef __BUILD_GTESTS__
std::string __event__last_data__;
std::string LoadLastEventSendData(void) { return __event__last_data__; };
#endif

inline void MQTTPub(const char *topic, std::string &value) {

};

inline void FlowPub(int type, const std::string &eventStr) {


    FlowRpcAsynProcessor_flow->SendToFlowServer(type, eventStr);
};

void saveCache(Event *e, const std::vector<uchar> &imageBuff,
               const std::string &ossPath) {

};

void resendCache(void);

bool uploadImageByBuff(Event *e, const std::vector<uchar> &imageBuff,
                       const std::string &ossPath, std::string &image_url,
                       bool saveToFile) {

};

bool uploadImage(Event *e, std::string &image_url, bool saveToFile = true) {

};

std::mutex resendCache_mt;
void resendCache(void) {
};

// NOTE: storage for track id && face id
class IDMAP {
 public:
    std::string &operator[](std::string &key) {
        std::lock_guard<std::mutex> gd(mt);
        return mp[key];
    };
    void erase(std::string key) {
        std::lock_guard<std::mutex> gd(mt);
        mp.erase(key);
    };
    int size(void) {
        std::lock_guard<std::mutex> gd(mt);
        return mp.size();
    };
    bool find(std::string &key, std::string &value) {
        std::lock_guard<std::mutex> gd(mt);
        bool ret = true;
        if (mp.count(key) > 0)
            value = mp[key];
        else
            ret = false;
        return ret;
    };

 private:
    std::mutex mt;
    std::unordered_map<std::string, std::string> mp;
} idmap;

inline std::string GenValue() { return ""; };

void ApproachEvent::handler() {

}

void WorkerEvent::handler() {
    Json::Value root ;

    root["isWorking"] = isWorking_;
    root["device_sn"] = cameraSn;
    root["timestamp"] = timeStamp;
    root["area_id"] = areaId_;

    std::string outputJson = Json::FastWriter().write(root);

    LOG(INFO) << "worker message: " << outputJson;
    FlowPub(8, outputJson);
}

void PassbyEvent::handler() {
}

void LoggerEvent::handler() {
    Json::Value data ;
    std::unordered_map<std::string, int> metrics;

    metrics = ServiceMetricManager::getAndClear(action_);

    data["time"] = timeStamp;
    data["action"] = static_cast<int>(action_);
    std::string image_url;
    float rate = 1000.0 / WHALE_ALGO_MODEL_METRIC_INTERVAL;
    bool upload_ok = false;

    switch (action_) {
        case MetricAction::kAlgoModelMonitor:
            for (auto &ite : metrics)
                if (ite.first == "cv_input_frame_count") {
                    data["cv_input_frame_rate"] = ite.second * rate;
                    LOG(INFO) << ite.first << ":" << ite.second;
                } else if (ite.first == "cv_output_frame_count") {
                    data["cv_output_frame_rate"] = ite.second * rate;
                    LOG(INFO) << ite.first << ":" << ite.second;
                } else
                    data[ite.first] = ite.second;
            break;
        case MetricAction::kAlgoDataMonitor:
            // when peoplecounting thread send passby data success,send this
            // metric
            for (auto &ite : metrics) data[ite.first] = ite.second;
            break;
        case MetricAction::kImageStatusMonitor:
            upload_ok = uploadImage(this, image_url);
            for (auto &ite : metrics) data[ite.first] = ite.second;
            data["cv_image_url"] = image_url;
            if (!upload_ok) {
                LOG(INFO) << "metric monitor image upload failed! ";
                return;
            }
            break;
        default:
            break;
    }
    data["camera_sn"] = cameraSn;
}

bool LoggerEvent::reSend(const std::vector<uchar> &imageBuff,
                         const std::string &ossPath) {
    Json::Value data;
    std::unordered_map<std::string, int> metrics;

    metrics = ServiceMetricManager::getAndClear(action_);

    data["time"] = timeStamp;
    data["action"] = static_cast<int>(action_);
    std::string image_url;
    float rate = 1000.0 / WHALE_ALGO_MODEL_METRIC_INTERVAL;
    bool upload_ok =
        uploadImageByBuff(this, imageBuff, ossPath, image_url, false);
    if (!upload_ok) {
        LOG(INFO) << "resend failed";
        return upload_ok;
    }
    for (auto &ite : metrics) data[ite.first] = ite.second;
    data["cv_image_url"] = image_url;
    data["camera_sn"] = cameraSn;

    return upload_ok;
};

void DoorEvent::handler() {

};

void FrontEvent::handler() {

};

bool FrontEvent::reSend(const std::vector<uchar> &imageBuff,
                        const std::string &ossPath) {
};

void FrontEvent::process(const std::string &image_url, bool retry) {

}

void FrontEvent::dispatch(const std::string &url, std::string &response,
                          bool retry) {

}

void FrontEvent::sendToFlow(const std::string &url,
                            const Json::Value &response) {

}


void FrontEvent::localAttribute() {

}

bool VehicleEvent::send(std::string image_url) {
    Json::Value vehicleValue;
    std::string response;
    vehicleValue["trackID"] = std::to_string(trackid_);
    vehicleValue["trackType"] = trackType_;
    vehicleValue["parkNum"] = park_num_;
    vehicleValue["parkId"] = park_id_;
    vehicleValue["cameraSn"] = cameraSn;
    vehicleValue["vehicleImg"] = image_url;
    vehicleValue["shop_id"] = GetGlobalVariable()->shop_id;
    vehicleValue["group"] = GetGlobalVariable()->group_name;

    std::string payload = Json::FastWriter().write(vehicleValue);
    return HttpPost(api_url_.c_str(), payload, response);
};

void VehicleEvent::handler() {
    // upload img to oss
    std::string image_url;
    bool upload_ok = uploadImage(this, image_url);

    if (upload_ok) {
        LOG(INFO) << "Vehicle img upload succeed,img url:" << image_url;

        auto ret = send(image_url);
        if (!ret) LOG(INFO) << "NOTE: send to monitor service error ";
    } else {
        LOG(INFO) << "upload image to oss failed!";
    }
};

bool VehicleEvent::reSend(const std::vector<uchar> &imageBuff,
                          const std::string &ossPath) {
    std::string image_url;
    bool upload_ok =
        uploadImageByBuff(this, imageBuff, ossPath, image_url, false);
    if (upload_ok) {
        LOG(INFO) << "Vehicle img Reupload succeed,img url:" << image_url;

        auto ret = send(image_url);
        if (!ret) {
            LOG(INFO) << "NOTE: resend to monitor service error ";
            return false;
        } else
            return true;
    } else {
        LOG(INFO) << "Reupload image to oss failed!";
        return false;
    }
};

void StallCustomerEvent::handler() {
    Json::Value root ;
    root["timestamp"] = std::to_string(timeStamp);
    root["device_sn"] = cameraSn;
    root["residence_time"] = std::to_string(residenceTime_);
    root["action"] = int(action_);
    std::string outputJson = Json::FastWriter().write(root);

    LOG(INFO) << "customer message: " << outputJson;
    FlowPub(9, outputJson);
};

void MonitorEvent::handler() {
    if (!x_axis.empty() && !y_axis.empty() && !id.empty()) {
        Json::Value monitorValue;
        std::string response;

        Json::Value crowd_info_vec = Json::arrayValue;
        for (int i = 0; i < x_axis.size(); ++i) {
            Json::Value crowd_info_map;
            crowd_info_map["x"] = x_axis[i];
            crowd_info_map["y"] = y_axis[i];
            crowd_info_map["id"]= id[i];

            crowd_info_vec.append(crowd_info_map);
        }

        monitorValue["shop_id"] = GetGlobalVariable()->shop_id;
        monitorValue["camera_sn"] = cameraSn;
        monitorValue["timestamp"] = timeStamp;
        monitorValue["Crowd"] = crowd_info_vec;

        std::string payload = Json::FastWriter().write(monitorValue);

        auto ret = HttpPost(api_url.c_str(), payload, response);
        if (!ret) LOG(INFO) << "NOTE: send to monitor service error ";
    }
};

}  // namespace vision
}  // namespace whale