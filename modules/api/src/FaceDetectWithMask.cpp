#include "FaceDetectWithMask.h"

#define kMaxFaceDetNum 48

namespace whale {
namespace core {

FaceDetectWithMask::FaceDetectWithMask(float height, float width,
                                       bool only_top1, bool soft_nms)
    : ProcessFactory("FaceDetectWithMask"),
      only_top1_(only_top1),
      soft_nms_(soft_nms),
      bboxes_(6 * kMaxFaceDetNum) {
    setShape(height, width);
    // generate anchor to predict box
    prepare_generate_anchor();
    output_tensor_names_ =
        std::vector<std::string>({"output1", "output2", "output3", "output4"});

    // get data dim
    float rect_area = height * width;
    int feature_size = (std::ceil(rect_area / 64) + std::ceil(rect_area / 256) +
                        std::ceil(rect_area / 1024)) *
                       2;
    dim_box[0] = 1;
    dim_box[1] = feature_size;
    dim_box[2] = 4;

    dim_cls[0] = 1;
    dim_cls[1] = feature_size;
    dim_cls[2] = 2;

    dim_mask[0] = 1;
    dim_mask[1] = feature_size;
    dim_mask[2] = 2;

    dim_land[0] = 1;
    dim_land[1] = feature_size;
    dim_land[2] = 10;
}

void FaceDetectWithMask::prepare_generate_anchor() {
    std::vector<std::vector<float>> min_sizes;
    std::vector<float> steps;

    float min_s[][3] = {{10, 20}, {32, 64}, {128, 256}};  // mobile
    float step_a[] = {8, 16, 32};
    for (int i = 0; i < 3; ++i) {
        steps.push_back(step_a[i]);
        std::vector<float> tmp;
        tmp.push_back(min_s[i][0]);
        tmp.push_back(min_s[i][1]);
        min_sizes.push_back(tmp);
        tmp.clear();
    }
    generate_anchors(min_sizes, steps);
}

void FaceDetectWithMask::generate_anchors(
    std::vector<std::vector<float>>& min_sizes, std::vector<float>& steps) {
    if (min_sizes.size() < 1 || steps.size() < 1) {
        LOG(FATAL) << "No input info for anchor generator, please check!";
        return;
    }

    if (min_sizes.size() != steps.size()) {
        LOG(FATAL) << "min_size.size() size must be equal to steps.size()!";
        return;
    }

    //得到原始feature坐标
    std::vector<std::vector<float>> feature_maps;
    for (int i = 0; i < steps.size(); ++i) {
        std::vector<float> tmp;
        tmp.push_back(std::ceil(input_height_ / steps[i]));
        tmp.push_back(std::ceil(input_width_ / steps[i]));
        feature_maps.push_back(tmp);
        tmp.clear();
    }
    //生成anchor
    for (int fDex = 0; fDex < feature_maps.size(); ++fDex) {
        int x_tmp = (int)feature_maps[fDex][0];  // x范围允许的最大值
        int y_tmp = (int)feature_maps[fDex][1];  // y范围允许的最大值

        std::vector<float> min_tmp = min_sizes[fDex];  //获取当前最小值
        for (int x = 0; x < x_tmp; ++x) {              //遍历x
            for (int y = 0; y < y_tmp; ++y) {          //遍历y
                for (int minDex = 0; minDex < min_tmp.size();
                     ++minDex) {                          //遍历size
                    if (-1 == min_tmp[minDex]) continue;  //跳过

                    std::vector<float> anchor_tmp;  //临时变量
                    anchor_tmp.push_back(((float)y + 0.5) * steps[fDex] /
                                         input_height_);  //中心
                    anchor_tmp.push_back(((float)x + 0.5) * steps[fDex] /
                                         input_width_);
                    anchor_tmp.push_back(min_tmp[minDex] /
                                         input_height_);  //框高
                    anchor_tmp.push_back(min_tmp[minDex] / input_width_);
                    priori_anchors_.push_back(anchor_tmp);
                    anchor_tmp.clear();
                }
            }
        }
        min_tmp.clear();
    }
}

FaceDetectWithMask::~FaceDetectWithMask() {}

void FaceDetectWithMask::preProcess(const cv::Mat& img) {
    if (img.empty()) {
        LOG(FATAL) << "image mat is empty!";
        return;
    }
    ori_height_ = (float)img.rows;
    ori_width_ = (float)img.cols;

    cv::Mat detect_img = img.clone();
    detect_img.convertTo(detect_img, CV_32FC3, 1, 0);
    cv::resize(detect_img, detect_img, cv::Size(input_width_, input_height_));

    std::vector<cv::Mat> channels;
    cv::split(detect_img, channels);
    float channel_mean_[3] = {104.0, 117.0, 123.0};
    float channel_scale_[3] = {1.0, 1.0, 1.0};

    for (int c = 0; c < 3; c++)
        channels[c] = (channels[c] - channel_mean_[c]) / channel_scale_[c];
    int index = 0;
    auto p_input_data_ = pre_data_.data();
    for (int c = 2; c >= 0; c--)  // RGB
        memcpy(p_input_data_ + (int)(c * input_height_ * input_width_),
               channels[c].data,
               (int)(input_height_ * input_width_) * sizeof(float));
}

void FaceDetectWithMask::postProcess(
    std::unordered_map<std::string, float*>& tensor_data_map,
    std::vector<whale::core::Box>& info) {
    // get data pointer
    float* box_point = tensor_data_map[output_tensor_names_[0]];
    float* cls_point = tensor_data_map[output_tensor_names_[1]];
    float* land_point = tensor_data_map[output_tensor_names_[2]];
    float* mask_point = tensor_data_map[output_tensor_names_[3]];
    postProcess(box_point, cls_point, land_point, mask_point, info);
}

void FaceDetectWithMask::postProcess(float* box_point, float* cls_point,
                                     float* land_point, float* mask_point,
                                     std::vector<whale::core::Box>& info) {
    //按分数过滤
    std::map<float, int, std::greater<float>> filtered_score_and_dex;
    for (int i = 0; i < dim_cls[1]; ++i) {
        if (cls_point[i * 2 + 1] > score_threshold_) {
            filtered_score_and_dex.insert(
                std::pair<float, int>(cls_point[i * 2 + 1], i));
        }
    }
    if (filtered_score_and_dex.size() < 1) return;  //没有人脸
    //转换box
    int max_num = only_top1_ ? 1 : filtered_score_and_dex.size();
    max_num = std::min(max_num, kMaxFaceDetNum);
    std::map<float, int>::iterator iter = filtered_score_and_dex.begin();
    for (int i = 0; i < max_num; ++i) {
        int dex = iter->second;
        float score = iter->first;
        iter++;
        float x1 = box_point[dex * 4 + 0] * 0.1 * priori_anchors_[dex][2] +
                   priori_anchors_[dex][0];
        float y1 = box_point[dex * 4 + 1] * 0.1 * priori_anchors_[dex][3] +
                   priori_anchors_[dex][1];
        float x2 =
            priori_anchors_[dex][2] * std::exp(0.2 * box_point[dex * 4 + 2]);
        float y2 =
            priori_anchors_[dex][3] * std::exp(0.2 * box_point[dex * 4 + 3]);
        x1 = (x1 - x2 / 2);
        y1 = (y1 - y2 / 2);
        x2 = (x2 + x1);
        y2 = (y2 + y1);

        x1 = x1 * ori_width_;
        y1 = y1 * ori_height_;
        x2 = x2 * ori_width_;
        y2 = y2 * ori_height_;
        bboxes_[i * 6 + 0] = x1;
        bboxes_[i * 6 + 1] = y1;
        bboxes_[i * 6 + 2] = x2;
        bboxes_[i * 6 + 3] = y2;
        bboxes_[i * 6 + 4] = score;
        bboxes_[i * 6 + 5] = dex;
    }
    filtered_score_and_dex.clear();
    // nms
    bool drop_flag[kMaxFaceDetNum] = {false};
    for (int i = 0; i < max_num; ++i) {
        if (drop_flag[i]) continue;
        float xi1 = clip_label_value(bboxes_[i * 6 + 0], ori_width_);
        float yi1 = clip_label_value(bboxes_[i * 6 + 1], ori_height_);
        float xi2 = clip_label_value(bboxes_[i * 6 + 2], ori_width_);
        float yi2 = clip_label_value(bboxes_[i * 6 + 3], ori_height_);
        float h1 = yi2 - yi1 + 1;
        float w1 = xi2 - xi1 + 1;
        if (h1 < ori_height_ * least_percent_of_edge_ ||
            w1 < ori_width_ * least_percent_of_edge_) {
            drop_flag[i] = true;
            // if(0 == i) return;
            continue;
        }
        float s1 = w1 * h1;
        for (int j = i + 1; j < max_num; ++j) {
            if (drop_flag[i]) continue;
            float xj1 = clip_label_value(bboxes_[j * 6 + 0], ori_width_);
            float yj1 = clip_label_value(bboxes_[j * 6 + 1], ori_height_);
            float xj2 = clip_label_value(bboxes_[j * 6 + 2], ori_width_);
            float yj2 = clip_label_value(bboxes_[j * 6 + 3], ori_height_);
            float h2 = yj2 - yj1 + 1;
            float w2 = xj2 - xj1 + 1;
            if (w2 < ori_width_ * least_percent_of_edge_ ||
                h2 < ori_height_ * least_percent_of_edge_) {
                drop_flag[i] = true;
                continue;
            }
            float s2 = w2 * h2;
            float xmin = xi1 > xj1 ? xi1 : xj1;
            float ymin = yi1 > yj1 ? yi1 : yj1;
            float xmax = xi2 > xj2 ? xj2 : xi2;
            float ymax = yi2 > yj2 ? yj2 : yi2;
            float s = (ymax - ymin + 1) * (xmax - xmin + 1);
            float ratio = s / (s1 + s2 - s);
            if (soft_nms_) {
                bboxes_[j * 6 + 4] =
                    bboxes_[j * 6 + 4] * exp(-2 * ratio * ratio);
                if (bboxes_[j * 6 + 4] < score_threshold_) drop_flag[j] = true;
            } else {
                if (ratio >= nms_threshold_) {
                    drop_flag[j] = true;
                }
            }
        }
    }
    for (int i = 0; i < max_num; ++i) {
        if (drop_flag[i]) continue;
        Box b;
        b.x0 = bboxes_[i * 6 + 0];
        b.y0 = bboxes_[i * 6 + 1];
        b.x1 = bboxes_[i * 6 + 2];
        b.y1 = bboxes_[i * 6 + 3];
        b.score = bboxes_[i * 6 + 4];
        int dex = bboxes_[i * 6 + 5];
        if (mask_point[dex * 2 + 1] >= mask_threshold_)
            b.with_mask = true;
        else
            b.with_mask = false;

        for (int j = 0; j < 5; ++j) {
            float x = land_point[dex * 10 + j * 2 + 0];
            float y = land_point[dex * 10 + j * 2 + 1];
            x = x * 0.1 * priori_anchors_[dex][2] + priori_anchors_[dex][0];
            y = y * 0.1 * priori_anchors_[dex][3] + priori_anchors_[dex][1];
            x = x * ori_width_;
            y = y * ori_height_;
            b.points.x[j] = clip_label_value(x, ori_width_);
            b.points.y[j] = clip_label_value(y, ori_height_);
        }
        info.push_back(b);
    }
}

float FaceDetectWithMask::clip_label_value(float x, float high) {
    return x < 0.0 ? 0.0 : (x >= high ? high : x);
}

}  // namespace core
}  // namespace whale