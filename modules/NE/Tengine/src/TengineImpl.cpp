#pragma once
#include <json/json.h>
#include "sdk.h"

namespace whale {
namespace core {

std::map<std::string, std::pair<EModelID, std::vector<std::string>>>
    DetectorCommonVmap = {
        {"PED_V3",
         {EModelID::e_ped_v3,
          {"det_people_v3_up.prototxt", "det_people_v3_up.caffemodel"}}},
        {"Monitor_V1",
         {EModelID::e_monitor_v1,
          {"det_tdpc_v1_up.prototxt", "det_tdpc_v1_up.caffemodel"}}},
        {"Huishoubao_V2",
         {EModelID::e_huishoubao_v2,
          {"ped_hsb_v6_up.prototxt", "ped_hsb_v6_up.caffemodel"}}},
        {"Capture_V2",
         {EModelID::e_capture_v2,
          {"det_capture_v2_up.prototxt", "det_capture_v2_up.caffemodel"}}},
        {"Vehicle_V1", {EModelID::e_vehicle_v1, {"PED_car_inst.wk"}}}};

namespace sdk {

void NNSDK::init(std::string model_path, const std::string &json) {
    LOG(INFO) << "NOTE: init Tengine SDK ";
    model_path += "/";

    Json::Value conf;
    Json::CharReaderBuilder jsoriReader;
    std::unique_ptr<Json::CharReader> const oriReader(jsoriReader.newCharReader());
    std::string err;
    oriReader->parse(json.c_str(), json.c_str()+json.size(), &conf, &err);


    LOG(INFO) << "NOTE: modules json is: " << conf;

    /* -- tengine --*/
    init_tengine();
    if (request_tengine_version("0.9") < 0) {
        printf("tengine version exception.\n");
        exit(-1);
    }
    /* -- attribute  */
    AttrImpl.init(
        "caffe",
        std::string(model_path + "face_attr_deploy96.prototxt").c_str(),
        std::string(model_path + "face_attr_deploy96.caffemodel").c_str());

    /*  -- FeatureReID -- */
    FReIDImpl.init(
        "caffe", std::string(model_path + "sqeezenet_new.prototxt").c_str(),
        std::string(model_path + "sqeezenet_new.caffemodel").c_str());

    /* -- Face Score -- */
    RscoreImpl.init(
        "caffe", std::string(model_path + "faceq200623_v1.prototxt").c_str(),
        std::string(model_path + "faceq200623_v1.caffemodel").c_str());
    BlurImpl.init(
        "caffe", std::string(model_path + "faceblur200623_v1.prototxt").c_str(),
        std::string(model_path + "faceblur200623_v1.caffemodel").c_str());
    OccludeImpl.init("caffe",
                     std::string(model_path + "face_occ.prototxt").c_str(),
                     std::string(model_path + "face_occ.caffemodel").c_str());
    PoseImpl.init("caffe", std::string(model_path + "fsanet.prototxt").c_str(),
                  std::string(model_path + "fsanet.caffemodel").c_str());

#ifdef __BUILD_FACEDOOR__
    FeatureImpl.init(
        "caffe", std::string(model_path + "facefeature_v2.prototxt").c_str(),
        std::string(model_path + "facefeature_v2.caffemodel").c_str());
    DetectWithMaskImpl.init("tengine",
                            std::string(model_path + "face_mask.tm").c_str());
#endif

    auto &data =
        DetectorCommonVmap[conf["Detect"]["det_model_version"].asString()];

    DetImpl.init(Json::FastWriter().write(conf["Detect"]), std::string("caffe"),
                 std::string(model_path + data.second[0]).c_str(),
                 std::string(model_path + data.second[1]).c_str());
};

void NNSDK::release() { release_tengine(); };

std::string NNSDK::getVersion() { return "0.0.0.0"; };

}  // namespace sdk
}  // namespace core
}  // namespace whale