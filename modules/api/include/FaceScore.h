#pragma once
#include <math.h>

#include <iostream>
#include <unordered_map>

#include "inference.h"

namespace whale {
namespace core {

inline cv::Mat cropAndResize(cv::Mat image, cv::Rect rect, cv::Size dsize) {
    cv::Mat tmp = image(rect);
    cv::resize(tmp, tmp, dsize);
    return tmp;
};

inline cv::Mat getImage(cv::Mat image, const cv::Rect &rect, int width,
                        int height) {
    cv::Rect r;
    r.x = std::max(int(0), int(rect.x));
    r.y = std::max(int(0), int(rect.y));
    r.width = std::min(int(image.cols - 1 - r.x), rect.width);
    r.height = std::min(int(image.rows - 1 - r.y), rect.height);

    return cropAndResize(image, r, cv::Size(width, height));
};

inline cv::Mat getImageRGB(cv::Mat image, const cv::Rect &rect, int width,
                           int height) {
    cv::Rect r;
    r.x = std::max(int(0), int(rect.x));
    r.y = std::max(int(0), int(rect.y));
    r.width = std::min(int(image.cols - 1 - r.x), rect.width);
    r.height = std::min(int(image.rows - 1 - r.y), rect.height);

    int ext_x0 = std::max(int(0), int(r.x - 0.6 * float(r.width)));
    int ext_y0 = std::max(int(0), int(r.y - 0.6 * float(r.height)));
    int ext_x1 = std::min(int(image.cols - 1), int(r.x + 1.6 * float(r.width)));
    int ext_y1 =
        std::min(int(image.rows - 1), int(r.y + 1.6 * float(r.height)));

    return cropAndResize(
        image, cv::Rect(cv::Point(ext_x0, ext_y0), cv::Point(ext_x1, ext_y1)),
        cv::Size(width, height));
};

class FaceScore : public ProcessFactory {
 public:
    FaceScore(std::string model_name = "FaceScore")
        : ProcessFactory(model_name) {
        setShape(64, 64);
        // output_tensor_names_ = std::vector<std::string>({"default"});
    };

    ~FaceScore(){};

    void postProcess(std::unordered_map<std::string, float *> &tensor_data_map,
                     const boost::any &any) {
        // one Box
        float *outdata = tensor_data_map["default"];
        FaceQuality *quality = boost::any_cast<FaceQuality *>(any);
        postProcess((float)outdata[0], quality);
    };

    void postProcess(float data, FaceQuality *quality) {
        // check init
        if (fabs(quality->blur + 1) < 0.001) {
            quality->blur = data;
            if (data < blur_thresh) quality->score = -2;
        } else if (fabs(quality->rscore + 1) < 0.001) {
            quality->rscore = data;
            if (data < rscore_thresh) quality->score = -3;
        }
    };

    void preProcess(const cv::Mat &image) {
        float norm_factor = 0.0039215686;  // 1 /255.0
        float *data = pre_data_.data();
        unsigned char *src_ptr = (unsigned char *)(image.ptr(0));
        int hw = input_height_ * input_width_;
        for (int h = 0; h < input_height_; h++) {
            for (int w = 0; w < input_width_; w++) {
                for (int c = 2; c >= 0; --c) {
                    data[(2 - c) * hw + h * input_width_ + w] =
                        norm_factor * (float)(*src_ptr);
                    ++src_ptr;
                }
            }
        }
    };

 private:
#ifndef __BUILD_FACEDOOR__
    float blur_thresh = 0.65;
    float rscore_thresh = 0.63;
#else
    float blur_thresh = 0.45;
    float rscore_thresh = 0.63;
#endif
};

class FaceScoreOcc : public ProcessFactory {
 public:
    FaceScoreOcc(std::string model_name = "FaceScoreOcc")
        : ProcessFactory(model_name) {
        setShape(64, 64);
        // output_tensor_names_ = std::vector<std::string>({"default"});
    };

    ~FaceScoreOcc(){};

    void postProcess(std::unordered_map<std::string, float *> &tensor_data_map,
                     const boost::any &any) {
        // one Box
        float *outdata = tensor_data_map["default"];
        FaceQuality *quality = boost::any_cast<FaceQuality *>(any);
        postProcess(outdata, quality);
    };

    void postProcess(float *occlude_data, FaceQuality *quality) {
        int not_occ_num = 0;
        for (int i = 8; i < 28; ++i) {
//            std::cout << occlude_data[i * 3 + 2] << " ";
            if (occlude_data[i * 3 + 2] < 0.5) {
                ++not_occ_num;
            }
        }
//        std::cout << "\n";
        quality->occlude = float(not_occ_num) / 20.0;
        if (quality->occlude < occ_thresh)
            quality->score = -4;
        else
            quality->score = quality->occlude;
    };

    void preProcess(const cv::Mat &image) {
        // float mean[3] = {123.675, 116.28, 103.53};	// 255 * [0.485, 0.456,
        // 0.406] float std[3] = {0.017125, 0.017507,
        // 0.017429};	// 1 / (255*[0.229, 0.224, 0.225])
        float mean[3] = {127.5, 127.5, 127.5};  // 1
        float std[3] = {0.0078431372549019607, 0.0078431372549019607,
                        0.0078431372549019607};  // 1 / 127.5

        float *data_rgb = pre_data_.data();
        unsigned char *src_ptr = (unsigned char *)(image.ptr(0));
        int hw = input_height_ * input_width_;
        for (int h = 0; h < input_height_; h++) {
            for (int w = 0; w < input_width_; w++) {
                for (int c = 2; c >= 0; --c) {
                    data_rgb[c * hw + h * input_width_ + w] =
                        std[c] * (float)(*src_ptr - mean[c]);
                    ++src_ptr;
                }
            }
        }
    };

 private:
#ifndef __BUILD_FACEDOOR__
    float occ_thresh = 0.75;
#else
    float occ_thresh = 0.75;
#endif
};

class FaceScorePose : public ProcessFactory {
 public:
    FaceScorePose(std::string model_name = "FaceScorePose")
        : ProcessFactory(model_name) {
        setShape(64, 64);
        // output_tensor_names_ = std::vector<std::string>({"default"});
    };

    ~FaceScorePose(){};

    void postProcess(std::unordered_map<std::string, float *> &tensor_data_map,
                     const boost::any &any) {
        // one Box
        float *outdata = tensor_data_map["default"];
        FaceQuality *quality = boost::any_cast<FaceQuality *>(any);
        postProcess(outdata, quality);
    };

    void postProcess(float *pose_data, FaceQuality *quality) {
        quality->yaw = pose_data[0];
        quality->pitch = pose_data[1];
        quality->roll = pose_data[2];

        float pose_score = 1.0 - (exp(fabs(float(quality->yaw)) / 30) +
                                  exp(fabs(float(quality->pitch)) / 30) +
                                  exp(fabs(float(quality->roll)) / 40) - 3) *
                                     0.2;
        if (pose_score < facepose_thresh) {
            quality->score = -5;
            return;
        }

        quality->score =
            quality->rscore * 0.4 + pose_score * 0.4 + quality->occlude * 0.2;
    };

    void preProcess(const cv::Mat &image) {
        // float mean[3] = {123.675, 116.28, 103.53};	// 255 * [0.485, 0.456,
        // 0.406] float std[3] = {0.017125, 0.017507,
        // 0.017429};	// 1 / (255*[0.229, 0.224, 0.225])

        float *data_rgb_ext = pre_data_.data();
        unsigned char *src_ext_ptr = (unsigned char *)(image.ptr(0));
        int hw = input_height_ * input_width_;
        for (int h = 0; h < input_height_; h++) {
            for (int w = 0; w < input_width_; w++) {
                for (int c = 2; c >= 0; --c) {
                    data_rgb_ext[c * hw + h * input_width_ + w] =
                        (float)(*src_ext_ptr - 127.5) * 0.0078125;
                    ++src_ext_ptr;
                }
            }
        }
    };

 private:
#ifndef __BUILD_FACEDOOR__
    float facepose_thresh = 0.7;
#else
    float facepose_thresh = 0.8;
#endif
};

}  // namespace core
}  // namespace whale