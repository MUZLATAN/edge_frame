#pragma once

#include "inference.h"

namespace whale {
namespace core {

inline cv::Mat getFaceAttrImage(const cv::Mat &image, int width, int height) {
    cv::Rect face_roi;
    face_roi.x = int(image.cols / 4);
    face_roi.y = int(image.rows / 4);
    face_roi.width = int(image.cols / 2);
    face_roi.height = int(image.rows / 2);

    cv::Mat img_tmp;
    img_tmp = image(face_roi);
    cv::resize(img_tmp, img_tmp, cv::Size(width, height));
    return img_tmp;
}

class FaceAttribute : public ProcessFactory {
 public:
    FaceAttribute(std::string model_name = "FaceAttribute")
        : ProcessFactory(model_name) {
        setShape(96, 96);
        output_tensor_names_ =
            std::vector<std::string>({"mask_prob", "fc_blob2", "age_prob"});
    };
    ~FaceAttribute() {}

    void preProcess(const cv::Mat &image) {
        int img_h = input_height_;
        int img_w = input_width_;

        float mean[3] = {0.0, 0.0, 0.0};
        float std[3] = {0.0039215686, 0.0039215686, 0.0039215686};  // 1 / 255

        // cv::Rect face_roi;
        // face_roi.x = int(image.cols / 4);
        // face_roi.y = int(image.rows / 4);
        // face_roi.width = int(image.cols / 2);
        // face_roi.height = int(image.rows / 2);

        // cv::Mat img_tmp;
        // img_tmp = image(face_roi);
        // cv::resize(img_tmp, img_tmp, cv::Size(img_w, img_h));

        auto img_tmp = image;

        unsigned char *src_ptr = (unsigned char *)(img_tmp.ptr(0));
        float *data = pre_data_.data();
        int hw = img_h * img_w;
        for (int h = 0; h < img_h; h++) {
            for (int w = 0; w < img_w; w++) {
                for (int c = 0; c < 3; c++) {
                    data[c * hw + h * img_w + w] =
                        std[c] * (float)(*src_ptr - mean[c]);
                    src_ptr++;
                }
            }
        }
    }

    void postProcess(std::unordered_map<std::string, float *> &tensor_data_map,
                     const boost::any &any) {
        float *mask_data = tensor_data_map["mask_prob"];
        float *gender_data = tensor_data_map["fc_blob2"];
        float *age_data = tensor_data_map["age_prob"];

        Attribute *attr = boost::any_cast<Attribute *>(any);
        postProcess(mask_data, gender_data, age_data, attr);
    }

    void postProcess(float *mask_data, float *gender_data, float *age_data,
                     Attribute *attr) {
        // int mask = mask_data[1] > mask_data[0] ? 1 : 0;

        // std::cout << " mask data " << mask_data[0] << "   " << mask_data[1]
        // 					<< " gender_data " <<
        // gender_data[0]
        // <<
        // "
        // "
        // << gender_data[1]
        // 					<< std::endl
        // 					<< "  age_data";
        // for (int i = 0; i < 7; i++) std::cout << "  " << age_data[i] << "  ";
        // std::cout << std::endl;

        int mask = mask_data[1] > 0.55 ? 1 : 0;
        int gender = gender_data[1] > gender_data[0] ? 1 : 0;
        float age_tmp = 0.0;
        float norm_sum = 0.0;
        for (int i = 0; i < 7; i++) {
            norm_sum += age_data[i];
            float cls = i > 0 ? float(i) : 0.5;
            age_tmp += cls * age_data[i];
        }
        age_tmp = age_tmp / norm_sum * 10;
        int age = int(age_tmp + 0.5);

        attr->mask = mask;
        attr->gender = gender;
        attr->age = age;
    }
};
}  // namespace core
}  // namespace whale