#include "Detect.h"


#include <json/json.h>

#define clip(x, y) (x < 0 ? 0 : (x > y ? y : x))

namespace whale {
namespace core {

void Detect::setJson(const std::string &det_param_json_) {
    Json::Value det_param_json;
    Json::CharReaderBuilder jsoriReader;
    std::unique_ptr<Json::CharReader> const oriReader(jsoriReader.newCharReader());
    std::string err;
    oriReader->parse(det_param_json_.c_str(), det_param_json_.c_str()+det_param_json_.size(), &det_param_json, &err);


    std::string det_model_version =
        det_param_json["det_model_version"].asString();
    score_threshold = det_param_json["det_score_thresh"].asDouble();
    iou_threshold = det_param_json["det_iou_thresh"].asDouble();

    // switch version
    auto &data = DetectorCommonVmap[det_model_version];
    EModelID version_ID = data.first;
    center_variance = 0.1;
    size_variance = 0.2;
    num_featuremap = 4;
    model_box_l_name = "cat_blob3";
    model_score_l_name = "softmax_blob1";
    input_width_ = 448;
    input_height_ = 256;
    num_anchors = 14280;
    featuremap_size = {{56, 28, 14, 7}, {32, 16, 8, 4}};
    min_boxes = {{{11.3135f, 22.6275f},
                  {14.2545f, 28.509f},
                  {17.9595f, 35.919f},
                  {16.0f, 16.0f},
                  {20.1585f, 20.1585f},
                  {25.3985f, 25.3985f}},
                 {{22.6275f, 45.255f},
                  {28.509f, 57.0175f},
                  {35.919f, 71.8375f},
                  {32.0f, 32.0f},
                  {40.3175f, 40.3175f},
                  {50.797f, 50.797f}},
                 {{45.255f, 90.5095f},
                  {57.0175f, 114.035f},
                  {71.8375f, 143.675f},
                  {64.0f, 64.0f},
                  {80.635f, 80.635f},
                  {101.5935f, 101.5935f}},
                 {{90.5095f, 181.0195f},
                  {114.035f, 228.07f},
                  {143.675f, 287.3505f},
                  {128.0f, 128.0f},
                  {161.27f, 161.27f},
                  {203.1875f, 203.1875f}}};
    switch (version_ID) {
        case EModelID::e_ped_v3: {
            num_of_class = 4;
            min_boxes = {{{10.5593f, 21.4542f},
                          {13.3042f, 27.0308f},
                          {16.7622f, 34.0565f},
                          {14.9333f, 15.1704f},
                          {18.8146f, 19.1132f},
                          {23.7053f, 24.0815f}},
                         {{21.1190f, 42.9084f},
                          {26.6084f, 54.0610f},
                          {33.5244f, 68.1126f},
                          {29.8667f, 30.3407f},
                          {37.6297f, 38.2270f},
                          {47.4105f, 48.1631f}},
                         {{42.2380f, 85.8164f},
                          {53.2163f, 108.1221f},
                          {67.0483f, 136.2252f},
                          {59.7333f, 60.6815f},
                          {75.2593f, 76.4539f},
                          {94.8206f, 96.3257f}},
                         {{84.4755f, 171.6333f},
                          {106.4327f, 216.2441f},
                          {134.0967f, 272.4508f},
                          {119.4667f, 121.3630f},
                          {150.5187f, 152.9079f},
                          {189.6417f, 192.6519f}}};
            // model_file = "det_people_v3_up.wk";
            break;
        }
        case EModelID::e_monitor_v1: {
            LOG(INFO) << "Init model param with MonitorV1";
            input_width_ = 640;
            input_height_ = 360;
            num_anchors = 28920;
            featuremap_size = {{80, 40, 20, 10}, {45, 23, 12, 6}};
            num_of_class = 2;
            // model_file = "det_tdpc_v1_up.wk";
            break;
        }
        case EModelID::e_capture_v2: {
            LOG(INFO) << "Init model param with CaptureV2";
            num_of_class = 4;
            // model_file = "det_capture_v2_up.wk";
            break;
        }
        case EModelID::e_huishoubao_v2: {
            LOG(INFO) << "Init model param with HuishoubaoV2";
            num_of_class = 2;
            // model_file = "ped_hsb_v6_up.wk";
            break;
        }
        case EModelID::e_vehicle_v1: {
            LOG(INFO) << "Init model param with VehicleV1";
            num_anchors = 7084;
            num_of_class = 2;
            // model_file = "PED_car_inst.wk";
            min_boxes = {{{16.0f, 16.0f}},
                         {{45.255, 22.627},
                          {57.018, 28.509},
                          {57.018, 28.509},
                          {71.838, 35.919},
                          {32.0, 32.0},
                          {40.317, 40.317},
                          {22.627, 45.255},
                          {28.509, 57.018},
                          {35.919, 71.838}},
                         {{90.51, 45.255},
                          {114.035, 57.018},
                          {143.675, 71.838},
                          {64.0, 64.0},
                          {80.635, 80.635},
                          {101.594, 101.594},
                          {45.255, 90.51},
                          {57.018, 114.035},
                          {71.838, 143.675}},
                         {{181.019, 90.51},
                          {228.07, 114.035},
                          {287.35, 143.675},
                          {128.0, 128.0},
                          {161.27, 161.27},
                          {203.187, 203.187},
                          {90.51, 181.019},
                          {114.035, 228.07},
                          {143.675, 287.35}}};
            break;
        }
        default: {
            LOG(ERROR) << "Unknown detect model version ID:"
                       << det_model_version;
            return;
        }
    }

    std::vector<std::vector<float>> shrinkage_size;
    std::vector<int> w_h_list;
    w_h_list = {input_width_, input_height_};
    for (int i = 0; i < 2; ++i) {
        std::vector<float> shrinkage_item;
        for (int j = 0; j < featuremap_size[i].size(); ++j) {
            shrinkage_item.push_back(w_h_list[i] / featuremap_size[i][j]);
        }
        shrinkage_size.push_back(shrinkage_item);
    }

    /* generate prior anchors */
    for (int index = 0; index < num_featuremap; index++) {
        float scale_w = input_width_ / shrinkage_size[0][index];
        float scale_h = input_height_ / shrinkage_size[1][index];
        for (int j = 0; j < featuremap_size[1][index]; j++) {
            for (int i = 0; i < featuremap_size[0][index]; i++) {
                float x_center = (i + 0.5) / scale_w;
                float y_center = (j + 0.5) / scale_h;

                for (int k = 0; k < min_boxes[index].size(); k++) {
                    float w = min_boxes[index][k][0] / input_width_;
                    float h = min_boxes[index][k][1] / input_height_;
                    priors.push_back({clip(x_center, 1), clip(y_center, 1),
                                      clip(w, 1), clip(h, 1)});
                }
            }
        }
    }
    /* generate prior anchors finished */
    setShape(input_height_, input_width_);
    output_tensor_names_ =
        std::vector<std::string>({model_box_l_name, model_score_l_name});
}

void Detect::preProcess(const cv::Mat &image) {
    image_h = image.rows;
    image_w = image.cols;
    cv::Mat img;
    cv::resize(image, img, cv::Size(input_width_, input_height_));
    input_transform(img, pre_data_.data(), input_height_, input_width_);
};

void Detect::postProcess(
    std::unordered_map<std::string, float *> &tensor_data_map,
    std::vector<whale::core::Box> &output_boxes) {
    float *pOutputData_box = tensor_data_map[model_box_l_name];
    float *pOutputData_score = tensor_data_map[model_score_l_name];
    postProcess(pOutputData_box, pOutputData_score, output_boxes);
};

void Detect::postProcess(float *pOutputData_box, float *pOutputData_score,
                         std::vector<whale::core::Box> &output_boxes) {
    // for (int i = 0; i < 10; i++) std::cout << " " << pOutputData_box[i];
    // std::cout << std::endl;
    // for (int i = 0; i < 10; i++) std::cout << " " << pOutputData_score[i];
    // std::cout << std::endl;
    // std::cout << score_threshold << std::endl;

    // std::cout << "Detect::postProcess" << std::endl;
    std::vector<std::vector<whale::core::Box>> proposal_boxes;
    generateBBox(proposal_boxes, pOutputData_box, pOutputData_score,
                 score_threshold);
    // std::cout << "Detect::genbox" << std::endl;

    // std::cout << " proposal_boxes " << proposal_boxes[0][0].x0 << " "
    //           << proposal_boxes[0][0].x1 << " " << proposal_boxes[0][0].y0
    //           << " " << proposal_boxes[0][0].y1 << std::endl;

    nms(proposal_boxes, output_boxes, NMSTYPE::hard_nms);
    // std::cout << "Detect::nms" << std::endl;
};

void Detect::generateBBox(
    std::vector<std::vector<whale::core::Box>> &proposal_boxes,
    const float *output_boxes, const float *out_score, float score_threshold) {
    // std::cout << " image_w " << image_w << "  image_h " << image_h <<
    // std::endl;

    for (int i = 1; i < num_of_class; i++) {
        std::vector<whale::core::Box> boxes;
        for (int j = 0; j < num_anchors; j++) {
            float score = out_score[j * num_of_class + i];
            if (score > score_threshold) {
                whale::core::Box box;
                float x_center =
                    output_boxes[j * 4] * center_variance * priors[j][2] +
                    priors[j][0];
                float y_center =
                    output_boxes[j * 4 + 1] * center_variance * priors[j][3] +
                    priors[j][1];
                float w =
                    exp(output_boxes[j * 4 + 2] * size_variance) * priors[j][2];
                float h =
                    exp(output_boxes[j * 4 + 3] * size_variance) * priors[j][3];

                box.x0 = clip(x_center - w / 2.0, 1) * image_w;
                box.y0 = clip(y_center - h / 2.0, 1) * image_h;
                box.x1 = clip(x_center + w / 2.0, 1) * image_w;
                box.y1 = clip(y_center + h / 2.0, 1) * image_h;
                box.score = clip(score, 1);
                box.class_idx = i;
                boxes.push_back(box);
            }
        }
        proposal_boxes.push_back(boxes);
    }
    /*
    if(proposal_boxes[0].size() > 0) {
            LOG(INFO) << "class num:" << proposal_boxes.size();
            LOG(INFO) << "proposal_boxes num:" << proposal_boxes[0].size();
    }
    */
};

void Detect::nms(std::vector<std::vector<whale::core::Box>> &proposal_boxes,
                 std::vector<whale::core::Box> &output_boxes, NMSTYPE type) {
    for (int c = 1; c < num_of_class; c++) {
        std::vector<whale::core::Box> input = proposal_boxes[c - 1];
        std::sort(input.begin(), input.end(),
                  [](const whale::core::Box &a, const whale::core::Box &b) {
                      return a.score > b.score;
                  });
        int box_num = input.size();
        std::vector<int> merged(box_num, 0);

        for (int i = 0; i < box_num; i++) {
            if (merged[i]) continue;
            std::vector<whale::core::Box> buf;

            buf.push_back(input[i]);
            merged[i] = 1;

            float h0 = input[i].y1 - input[i].y0 + 1;
            float w0 = input[i].x1 - input[i].x0 + 1;

            float area0 = h0 * w0;

            for (int j = i + 1; j < box_num; j++) {
                if (merged[j]) continue;

                float inner_x0 =
                    input[i].x0 > input[j].x0 ? input[i].x0 : input[j].x0;
                float inner_y0 =
                    input[i].y0 > input[j].y0 ? input[i].y0 : input[j].y0;

                float inner_x1 =
                    input[i].x1 < input[j].x1 ? input[i].x1 : input[j].x1;
                float inner_y1 =
                    input[i].y1 < input[j].y1 ? input[i].y1 : input[j].y1;

                float inner_h = inner_y1 - inner_y0 + 1;
                float inner_w = inner_x1 - inner_x0 + 1;

                if (inner_h <= 0 || inner_w <= 0) continue;

                float inner_area = inner_h * inner_w;

                float h1 = input[j].y1 - input[j].y0 + 1;
                float w1 = input[j].x1 - input[j].x0 + 1;

                float area1 = h1 * w1;

                float score;

                score = inner_area / (area0 + area1 - inner_area);

                if (score > iou_threshold) {
                    merged[j] = 1;
                    buf.push_back(input[j]);
                }
            }
            switch (type) {
                case NMSTYPE::hard_nms: {
                    Box b;
                    b.x0 = buf[0].x0;
                    b.y0 = buf[0].y0;
                    b.x1 = buf[0].x1;
                    b.y1 = buf[0].y1;
                    b.score = buf[0].score;
                    b.class_idx = c;
                    output_boxes.push_back(b);
                    break;
                }
                case NMSTYPE::blending_nms: {
                    float total = 0;
                    for (int i = 0; i < buf.size(); i++) {
                        total += exp(buf[i].score);
                    }
                    Box rects;
                    memset(&rects, 0, sizeof(rects));
                    for (int i = 0; i < buf.size(); i++) {
                        float rate = exp(buf[i].score) / total;
                        rects.x0 += buf[i].x0 * rate;
                        rects.y0 += buf[i].y0 * rate;
                        rects.x1 += buf[i].x1 * rate;
                        rects.y1 += buf[i].y1 * rate;
                        rects.score += buf[i].score * rate;
                    }
                    rects.class_idx = c;
                    output_boxes.push_back(rects);
                    break;
                }
                default: {
                    printf("wrong type of nms.");
                    exit(-1);
                }
            }
        }
    }
};

void Detect::input_transform(cv::Mat &image, float *input_data, int img_h,
                             int img_w) {
    unsigned char *src_ptr = (unsigned char *)(image.ptr(0));
    int hw = img_h * img_w;

    for (int h = 0; h < img_h; h++) {
        for (int w = 0; w < img_w; w++) {
            // convert BGR2RGB
            input_data[2 * hw + h * img_w + w] =
                (*src_ptr - means[0]) * 3.921e-3;
            src_ptr++;
            input_data[1 * hw + h * img_w + w] =
                (*src_ptr - means[1]) * 3.921e-3;
            src_ptr++;
            input_data[h * img_w + w] = (*src_ptr - means[2]) * 3.921e-3;
            src_ptr++;
        }
    }
};

}  // namespace core
}  // namespace whale
