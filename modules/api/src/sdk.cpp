
#include "sdk.h"

#include "interface.h"

namespace whale {
namespace core {
namespace sdk {

static NNSDK nnsdk;

void init(const std::string &model_path, std::string json) {
    int app_sdk_type = 0;
    LOG(INFO) << "model_path: " << model_path << " ";
    nnsdk.init(model_path, json);
}

void detect(std::shared_ptr<whale::core::BaseFrame> frame,
            std::vector<Box> &boxes) {
#ifdef __BUILD_FACEDOOR__
    nnsdk.DetectWithMaskImpl.run(frame, boxes);
#else
    nnsdk.DetImpl.run(frame, boxes);

    // auto image = frame->cvImage.clone();
    // for (auto &box : boxes)
    //     cv::rectangle(
    //         image, cv::Rect(box.x0, box.y0, box.x1 - box.x0, box.y1 -
    //         box.y0), cv::Scalar(255, 255, 0));
    // cv::imwrite("testF.jpg", image);
#endif
}

void score(const cv::Mat &image, const cv::Rect &rect,
           FaceQuality &image_score) {
    FaceScore fs;
    int width, height;
    fs.getShape(height, width);
    auto img_norm = getImage(image, rect, width, height);
    nnsdk.BlurImpl.run(img_norm, &image_score);
    if (image_score.score < -1) return;

    nnsdk.RscoreImpl.run(img_norm, &image_score);
    if (image_score.score < -1) return;

    nnsdk.OccludeImpl.run(img_norm, &image_score);
    if (image_score.score < -1) return;

    FaceScorePose fsp;
    fsp.getShape(height, width);

    auto img_ext_norm = getImageRGB(image, rect, width, height);
    nnsdk.PoseImpl.run(img_ext_norm, &image_score);
}

void reid(cv::Mat const &frame, std::vector<float> &feature) {
    nnsdk.FReIDImpl.run(frame, &feature);
}

void attribute(const cv::Mat &image, Attribute &attri) {
    FaceAttribute fattr;
    int width, height;
    fattr.getShape(height, width);

    auto tmp = getFaceAttrImage(image, width, height);
    nnsdk.AttrImpl.run(tmp, &attri);
}

void facefeature(const cv::Mat &image, float *feature) {
#ifdef __BUILD_FACEDOOR__
    nnsdk.FeatureImpl.run(image, feature);
#endif
};
void release() { nnsdk.release(); }

std::string getversion() { return nnsdk.getVersion(); }
}  // namespace sdk
}  // namespace core
}  // namespace whale
