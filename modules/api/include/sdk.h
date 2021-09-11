
#pragma once
#include <memory>
#include <string>

#include "common.h"

namespace whale {
namespace core {
namespace sdk {

class NNSDK {
 public:
    void init(std::string model_path, const std::string &json);
    void release();
    std::string getVersion();

 public:
    Impl<DetectImpl> DetImpl;
    Impl<FaceAttributeImpl> AttrImpl;
    Impl<FeatureReIDImpl> FReIDImpl;

    Impl<FaceeScoreImpl> RscoreImpl;
    Impl<FaceeScoreImpl> BlurImpl;
    Impl<FaceOccludeImpl> OccludeImpl;
    Impl<FacePoseImpl> PoseImpl;

#ifdef __BUILD_FACEDOOR__
    Impl<FaceFeatureImpl> FeatureImpl;
    Impl<FaceDetectWithMaskImpl> DetectWithMaskImpl;
#endif
};

}  // namespace sdk
}  // namespace core
}  // namespace whale