#pragma once


#include <opencv2/opencv.hpp>
#include <json/json.h>

#include "ServiceMetricManager.h"
#include "common.h"
#include "mgr/ConfigureManager.h"

namespace whale {
namespace vision {

enum class EventType {
    kDefaultEvent = 0,  // 0
    kApproachEvent,
    kPassbyEvent,
    kFrontEvent,
    kFaceRegisterEvent,
    kWorkerAreaEvent,
    kLoggerEvent,
    kDoorEvent,
    kDisAppear,
    kVehicleEvent,
    kStallCustomerEvent,
    kMonitorEvent,
};

enum class RecognitionType {
    kGloble,
    kApp,
};

enum class Category {
    Category_UNKNOWN,
    TRAFFIC,
    POSITION,
};

//进店 出店 离开 type 与flow.proto中的stallType对应
enum class StallCustomerAction {
    CUSTOMER_UNKNOW,  // invalid
    CUSTOMER_IN,      // customer enter
    CUSTOMER_OUT,     // customer leave
    CUSTOMER_PASSBY   // customer passby
};

enum class Action {
    Action_UNKNOWN,  // invalid
    Action_IN,       // customer enter
    Action_OUT,      // customer leave
    Action_PASSBY,   // customer passby
};

class Event {
 public:
    template <typename... Type>
    Event(Type &&... value) {
        Event(std::forward<Type>(value)...);
    };

    Event(EventType e, int64_t camera_time = 0, std::string camera_sn = "")
        : et(e), timeStamp(camera_time), cameraSn(camera_sn){};
    virtual ~Event() {}
    virtual void handler() = 0;

    std::string toString() const {
        switch (et) {
            case EventType::kFrontEvent:
            case EventType::kFaceRegisterEvent:
                return "face";
            case EventType::kDoorEvent:
                return "door";
            case EventType::kPassbyEvent:
                return "body";
            case EventType::kLoggerEvent:
                return "monitor";
            case EventType::kApproachEvent:
                return "approach";
            case EventType::kDisAppear:
                return "disappear";
            default:
                return "default";
        }
    };
    virtual void getUploadImage(std::vector<uchar> &imageBuff,
                                std::vector<int> &param, std::string &ossPath) {
        LOG(FATAL) << "ERROR: cannot use this func";
    };

 public:
    EventType et;
    int64_t timeStamp;
    std::string cameraSn;
};

class ApproachEvent : public Event {
 public:
    ApproachEvent(int64_t time_stamp, const std::string &camera_sn,
                  const std::string &event_type)
        : Event(EventType::kApproachEvent, time_stamp, camera_sn),
          event_type_(event_type){};
    void handler();

 public:
    std::string event_type_;
};

class FrontEvent : public Event {
 public:
    FrontEvent(int id, int64_t face_approach_time, int64_t face_front_time,
               cv::Mat image, const std::string &camera_sn, int score = -1)
        : Event(EventType::kFrontEvent, face_approach_time, camera_sn),
          id_(id),
          face_front_time_(face_front_time),
          image_(image),
          score_(score) {
        api_url = ConfigureManager::instance()->getAsString("whale_face_api") +
                  "/recognizeAndRegist";
    };
    FrontEvent() : Event(EventType::kFrontEvent) {
        api_url = ConfigureManager::instance()->getAsString("whale_face_api") +
                  "/recognizeAndRegist";
    };
    void handler();


    bool reSend(const std::vector<uchar> &imageBuff,
                const std::string &ossPath);

    void getUploadImage(std::vector<uchar> &imageBuff, std::vector<int> &param,
                        std::string &ossPath) {
        cv::imencode(".jpg", image_, imageBuff, param);
        ossPath = GetGlobalVariable()->client_sn + "/" + cameraSn + "/image/" +
                  toString() + "/" + std::to_string(timeStamp) + "_" +
                  std::to_string(id_) + "_" + std::to_string(score_) + ".jpg";
    };

 private:
    void process(const std::string &image_url, bool retry = false);
    void sendToPad(const std::string &faceid);
    void dispatch(const std::string &url, std::string &response,
                  bool retry = false);
    void sendToFlow(const std::string &url, const Json::Value &response);

    void localAttribute();

 public:
    std::string api_url;
    int id_;
    int64_t face_front_time_;
    cv::Mat image_;
    int score_;
};

class DoorEvent : public Event {
 public:
    DoorEvent(const std::string &user_name, const std::string &user_avatar,
              int id, const cv::Mat &image, int64_t time_stamp,
              const std::string &camera_sn)
        : Event(EventType::kDoorEvent, time_stamp, camera_sn),
          id_(id),
          user_avatar_(user_avatar),
          user_name_(user_name),
          image_(image){};
    DoorEvent() : Event(EventType::kDoorEvent){};
    void handler();


    void getUploadImage(std::vector<uchar> &imageBuff, std::vector<int> &param,
                        std::string &ossPath) {
        cv::imencode(".jpg", image_, imageBuff, param);
        ossPath = GetGlobalVariable()->client_sn + "/" + cameraSn + "/image/" +
                  toString() + "/" + std::to_string(timeStamp) + "_" +
                  std::to_string(id_) + ".jpg";
    };

 public:
    int id_;
    cv::Mat image_;
    std::string user_avatar_;
    std::string user_name_;
};

// 人数统计功能专用
class PassbyEvent : public Event {
 public:
    PassbyEvent(int64_t id, int64_t face_approach_time,
                int64_t body_approach_time, int64_t face_front_time,
                int64_t residence_time, const std::string &camera_sn)
        : Event(EventType::kPassbyEvent, face_approach_time, camera_sn),
          id_(id),
          body_approach_time_(body_approach_time),
          face_front_time_(face_front_time),
          residence_time_(residence_time){};
    void handler();

 public:
    int64_t id_;
    int64_t body_approach_time_;
    int64_t face_front_time_;
    int64_t residence_time_;
};

class LoggerEvent : public Event {
 public:
    LoggerEvent(const std::string &camera_sn, const cv::Mat &image,
                int64_t camera_time, MetricAction id,
                std::string prefix = "monitor")
        : Event(EventType::kLoggerEvent, camera_time, camera_sn),
          camera_image_(image),
          prefix_(prefix),
          action_(id){};
    LoggerEvent(const std::string &camera_sn, int64_t camera_time,
                MetricAction id)
        : Event(EventType::kLoggerEvent, camera_time, camera_sn), action_(id){};
    LoggerEvent() : Event(EventType::kLoggerEvent){};
    void handler();
    bool reSend(const std::vector<uchar> &imageBuff,
                const std::string &ossPath);



    void getUploadImage(std::vector<uchar> &imageBuff, std::vector<int> &param,
                        std::string &ossPath) {
        cv::imencode(".jpg", camera_image_, imageBuff, param);

    };

 public:
    std::string prefix_ = "monitor";
    cv::Mat camera_image_;
    MetricAction action_;
};

class WorkerEvent : public Event {
 public:
    WorkerEvent(const std::string &camera_sn, bool &isWorking,
                int64_t camera_time, int id)
        : Event(EventType::kWorkerAreaEvent, camera_time, camera_sn),
          isWorking_(isWorking),
          areaId_(id){};
    void handler();

 public:
    bool isWorking_;
    int areaId_;
};

class VehicleEvent : public Event {
 public:
    VehicleEvent(std::string &cameraSn, int64_t camera_time,
                 std::string &trackType, cv::Mat &vehicle_image,
                 unsigned int aid, unsigned int anum, unsigned int tid)
        : Event(EventType::kVehicleEvent, camera_time, cameraSn),
          trackType_(trackType),
          vehicle_image_(vehicle_image),
          park_id_(aid),
          park_num_(anum),
          trackid_(tid) {
        api_url_ = ConfigureManager::instance()->getAsString("vehicle_api");
        LOG(INFO) << "get api url:" << api_url_;
    };
    VehicleEvent() : Event(EventType::kVehicleEvent) {
        api_url_ = ConfigureManager::instance()->getAsString("vehicle_api");
        LOG(INFO) << "get api url:" << api_url_;
    };

    bool send(std::string image_url);
    bool reSend(const std::vector<uchar> &imageBuff,
                const std::string &ossPath);

    void handler();
    void getUploadImage(std::vector<uchar> &imageBuff, std::vector<int> &param,
                        std::string &ossPath) {
        cv::imencode(".jpg", vehicle_image_, imageBuff, param);
        ossPath = GetGlobalVariable()->client_sn + "/" + cameraSn + "/image/" +
                  toString() + "/" + std::to_string(timeStamp) + "_" +
                  std::to_string(trackid_) + ".jpg";
    };

 public:
    std::string api_url_;
    std::string trackType_;
    cv::Mat vehicle_image_;
    unsigned int park_num_;
    unsigned int park_id_;
    unsigned int trackid_;
};

class StallCustomerEvent : public Event {
 public:
    StallCustomerEvent(StallCustomerAction action, const std::string &cameraSn,
                       int64_t residence_time, int64_t camera_time, int trackId)
        : Event(EventType::kStallCustomerEvent, camera_time, cameraSn),
          action_(action),
          residenceTime_(residence_time),
          trackId_(trackId){};
    void handler();

 public:
    StallCustomerAction action_;
    int64_t trackId_;
    int64_t residenceTime_;
};

class MonitorEvent : public Event {
 public:
    MonitorEvent(const std::string &cameraSn, std::vector<int> &x_axis,
                 std::vector<int> &y_axis, std::vector<int> &id,
                 int64_t camera_time)
        : Event(EventType::kMonitorEvent, camera_time, cameraSn),
          x_axis(x_axis),
          y_axis(y_axis),
          id(id) {
        api_url = ConfigureManager::instance()->getAsString("monitor_api");
        //        LOG(INFO) << "MonitorEventHandler get api url:" << api_url;
    };
    void handler();

 public:
    std::string api_url;
    std::vector<int> x_axis;
    std::vector<int> y_axis;
    std::vector<int> id;
};

#ifdef __BUILD_GTESTS__
std::string LoadLastEventSendData(void);
#endif

}  // namespace vision
}  // namespace whale
