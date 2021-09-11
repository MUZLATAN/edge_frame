#pragma once

#include <chrono>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

#include "AlgoData.h"

#ifdef __PLATFORM_HISI__
#include "IVE/IVEData.h"
#endif

#ifndef __TYPEDEFINE_WHALE_FRAME__
#define __TYPEDEFINE_WHALE_FRAME__
namespace whale {
namespace core {
#ifdef __PLATFORM_HISI__
typedef HISI::IVEFrame WhaleFrame;
#else
typedef BaseFrame WhaleFrame;
#endif
}  // namespace core
}  // namespace whale
#endif

#define WHALE_ALGO_MODEL_METRIC_INTERVAL \
    600000  //"After each 10*60*1000 ms, algo upload metric data to server"
#define WHALE_HEARTBEAT_INTERVAL \
    30000  //	"After each 30*1000 ms, it(rtsp/usb) shows a heartbreak to "
           //"server if healthy");
#define WHALE_DEVICE_GROUP "group_name"  // device group name
#define WHALE_SYS_PATH "sys_path"        // system path

#define VEHICLE_RECORD_TIME 0        // vehicle_record_time
#define CAPTURE_VISUALIZE false      // visualize Capture processer
#define CAPTURE_DEBUG_RUNTIME false  // debug Capture processer runtime
#define WORKER_VISUAL_DETAIL false   // debug worker detail info

// NOTE: storage for processor name
#define WHALE_PROCESSOR_MONITOR "MonitorProcessor"
#define WHALE_PROCESSOR_DETECT "DetectorProcessor"
#define WHALE_PROCESSOR_DISPATCH "DispatchProcessor"
#define WHALE_PROCESSOR_RTSP "RtspLoaderProcessor"
#define WHALE_PROCESSOR_USBCAMERA "UsbDeviceLoaderProcessor"
#define WHALE_PROCESSOR_HISICAMERA "HisiCameraProcessor"
#define WHALE_PROCESSOR_LOCALVIDEO "LocalVideoLoaderProcessor"
#define WHALE_PROCESSOR_FLOWRPC "FlowRpcAsynProcessor"
#define WHALE_PROCESSOR_WEBSOCKETSTREAM "WebsocketStreamProcessor"

// NOTE: openplatform info
#if defined __PLATFORM_ARM__
    #define WHALE_HEARBEAT_URL "http://192.168.2.192:9898/heart"
#elif defined __PLATFORM_HISI__
    #define WHALE_HEARBEAT_URL "http://127.0.0.1:9898/heart"
#else
    #define WHALE_HEARBEAT_URL "https://proxy.meetwhale.com/chunnel/heartbeat/check/v12"
#endif

#define WHALE_GROUPINFO_URL \
    "http://api.meetwhale.com/device/getgroupinfo?sensor_sn="
#define WHALE_OTAVERSION_URL "https://proxy.meetwhale.com/edgeota/version"


#if defined __PLATFORM_ARM__
  #define WHALE_IOT_REGISTER_URL "http://192.168.2.192:9898/register"
#elif defined __PLATFORM_HISI__
  #define WHALE_IOT_REGISTER_URL "http://127.0.0.1:9898/register"
#else
  #define WHALE_IOT_REGISTER_URL "http://api.develop.meetwhale.com/v1/device/batcherregisterhub"
#endif

// NOTE: face api
#define WHALE_FACEAPI_PRODUCT_URL "cv.meetwhale.com:5000"
#define WHALE_FACEAPI_DEV_URL "47.103.31.235:5000"

// NOTE: justli: BACK END
#define WHALE_FLOW_PRODUCT_URL "https://flow.meetwhale.com/api/v1/event"
#define WHALE_FLOW_DEV_URL "http://flow.develop.meetwhale.com:8443/api/v1/event"
#define WHALE_CHUNNEL_PRODUCT_URL "http://proxy.meetwhale.com/chunnel/create"
#define WHALE_CHUNNEL_DEV_URL \
    "http://proxy.develop.meetwhale.com/chunnel/create"

// NOTE: REMOTE CONFIG, ADAM
#define WHALE_REMOTE_CONFIG_PRODUCT_URL                  \
    "https://data-hiti-http.data.meetwhale.com/hiti/key/" \
    "conf?app_name=AlgoConf&sn="
#define WHALE_REMOTE_CONFIG_DEV_URL                      \
    "https://data-hiti-http.data.meetwhale.com/hiti/key/" \
    "conf_dev?app_name=AlgoConf&sn="
#define WHALE_REMOTE_VIDEO_CONFIG_PRODUCT_URL            \
    "https://data-hiti-http.data.meetwhale.com/hiti/key/" \
    "record?app_name=videoReocrd&sn="
#define WHALE_REMOTE_VIDEO_CONFIG_DEV_URL                \
    "https://data-hiti-http.data.meetwhale.com/hiti/key/" \
    "record_dev?app_name=videoReocrd&sn="

#define WHALE_HTTPSERVER_URL "http://0.0.0.0:8088/face"

// NOTE: MQTT
#define WHALE_MQTT_BROKER_PRODUCT_URL "192.168.2.192"
#define WHALE_MQTT_BROKER_DEV_URL "mqtt.develop.meetwhale.com"
#define WHALE_MQTT_PORT "1883"

// NOTE: OSS
#define WHALE_OSS_PRODUCT_URL "oss-cn-hangzhou.aliyuncs.com"
#define WHALE_OSS_DEV_URL "oss-cn-beijing.aliyuncs.com"
#define WHALE_OSS_BUCKET_PRODUCT_BUCKET "whale-cv-prod"
#define WHALE_OSS_BUCKET_DEV_BUCKET "whale-cv-test"

namespace whale {
namespace vision {

struct WhaleData {
 public:
    WhaleData(){};
    // WhaleData(const WhaleData& data){};
    WhaleData(std::vector<whale::core::Box>& boxes,
              std::shared_ptr<whale::core::WhaleFrame> frame)
        : object_boxes(boxes), frame_(frame){};

 public:
    // int64_t camera_time = 0;
    // std::string camera_sn;
    // unsigned long long frame_id = 0;
    std::vector<whale::core::Box> object_boxes;
    std::shared_ptr<whale::core::WhaleFrame> frame_;
};

// storage for gloabl variable, just like "camera sn"
struct GlobalVariable {
 public:
    std::string oss_base_path = "";
    std::string face_pad_topic = "";
    std::string algo_version = "";
    std::string sys_version = "";
    std::string sys_path = "";
    std::string client_sn = "";
    std::string camera_sns = "";    // Maybe multiple
    std::string video_inputs = "";  // Maybe multiple
    std::string group_id = "";
    std::string company_id = "";
    std::string shop_id = "";
    std::string group_name = "";
    std::string route_sn = "";

    // value
    std::string chunnel_endpoint = "";
    std::string flowrpc_url = "";
    bool isDevEnv = false;

    bool pad_context = false;
    bool re_snapshot = false;
    bool stream_connect = false;
    bool tcp_stream_connect = false;
    bool sys_quit = false;
    bool isLocalInput = false;
};

GlobalVariable* GetGlobalVariable(void);

}  // namespace vision
}  // namespace whale
