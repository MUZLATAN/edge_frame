#include "processor/DetectorProcessor.h"

#include <algorithm>
#include <thread>

#include "ServiceMetricManager.h"
#include "SysPublicTool.h"
#include "event/Event.h"
#include "interface.h"
#include "mgr/ConfigureManager.h"
#include "mgr/QueueManager.h"
#include "util.h"

using namespace whale::core;

namespace whale {
namespace vision {

const unsigned char LabelColors[][3] = {
    {2, 224, 17}, {251, 144, 17}, {206, 36, 255}, {247, 13, 145}, {0, 78, 255}};

void DetectorProcessor::init() {
    QueueManager::SafeGet(processor_name_, input_queue_);
    QueueManager::SafeGet(WHALE_PROCESSOR_DISPATCH, output_queue_);

    std::vector<std::string> features =
        ConfigureManager::instance()->getAsVectorString("feature");
    for (int i = 0; i < features.size(); ++i) {
        std::string base_processor = feature_map[features[i]][0];
        QueuePtr q;
        QueueManager::SafeGet(base_processor, q);
        next_processor_unit_[base_processor] = q;
        LOG(INFO) << base_processor << " add to detectorprocessor next queue. ";
    }

    last_image_monitor_time_ = getCurrentTime();
    last_model_metric_monitor_time_ = last_image_monitor_time_;

    double image_monitor_interval_in =
        ConfigureManager::instance()->getAsDouble("image_monitor_interval");
    image_monitor_interval_in = std::max(image_monitor_interval_in, 0.01);
    image_monitor_interval = image_monitor_interval_in * 60 * 1000;
}

bool EndsWith(std::string const& s, std::string const& ending) {
    if (s.length() < ending.length()) return false;
    return (0 ==
            s.compare(s.length() - ending.length(), ending.length(), ending));
}

void GetDateTime(std::string& date, std::string& time) {
    auto t =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%m-%d ");
    date = ss.str();

    std::stringstream ss2;
    ss2 << std::put_time(std::localtime(&t), "%A");
    date += ss2.str().substr(0, 3);

    std::stringstream ss3;
    ss3 << std::put_time(std::localtime(&t), "%T");
    time = ss3.str();
}

void GetTimeGreetings(const std::string& time, std::string& greet) {
    int hour = std::stoi(time.substr(0, 2));
    if ((5 <= hour) && (hour < 12)) {
        greet = "Good morning";
    } else if ((12 <= hour) && (hour < 18)) {
        greet = "Good afternoon";
    } else if ((18 <= hour) && (hour < 21)) {
        greet = "Good evening";
    } else {
        greet = "Good night";
    }
}

void DetectorProcessor::labelBox(cv::Mat& rgbImage,
                                 std::vector<Box>& det_results) {
    if (frame_idx_ < 0) {
        // init, read template
        std::string sys_path =
            ConfigureManager::instance()->getAsString("sys_path");
        std::string template_path = sys_path + "/templates/";

        template_ = cv::imread(template_path + "template.jpg");
        if (!template_.data) {
            LOG(INFO) << "error reading template, using black background\n";
            template_ = cv::Mat::zeros(960, 540, CV_8UC3);
        }
        cv::resize(template_, template_,
                   cv::Size(rgbImage.cols, rgbImage.rows));

        template2_ = cv::imread(template_path + "template2.jpg");
        if (!template2_.data) {
            LOG(INFO) << "error reading template2, using default\n";
            template2_ = template_;
        }
        cv::resize(template2_, template2_,
                   cv::Size(rgbImage.cols, rgbImage.rows));

        for (int i = 0; i < 101; ++i) {
            std::stringstream ss;
            ss << std::setw(2) << std::setfill('0') << i;
            auto c =
                cv::imread(template_path + "circle/CCOO" + ss.str() + ".jpg");
            if (!c.data) continue;
            circles_.push_back(c);
        }
    }
    ++frame_idx_;

    bool has_face = false;
    int max_area = -1;

    unsigned int color_idx = 0;
    for (const auto& box : det_results) {
        if (box.class_idx != cls_face) {
            continue;
        }

        has_face = true;
        int area = int(box.x1 - box.x0) * int(box.y1 - box.y0);
        if (area > max_area) {
            max_area = area;
        }

        color_idx++;

        typedef cv::Point Point;
        cv::Rect loc = box.rect;
        char label_id[256] = {0};

        if (circles_.size() > 0) {
            Point pt1(box.x0, box.y0);
            Point pt2(box.x1, box.y1);
            cv::Rect r(pt1, pt2);
            int d = int((r.width + r.height) * 0.56);
            int radius = d >> 1;
            int cx = (pt1.x + pt2.x) >> 1;
            int cy = (pt1.y + pt2.y) >> 1;

            int idx = frame_idx_ % circles_.size();
            cv::Mat re, re_mask;
            cv::resize(circles_[idx], re, cv::Size(d, d));
            cv::cvtColor(re, re_mask, cv::COLOR_BGR2GRAY);
            cv::threshold(re_mask, re_mask, 150, 255, cv::THRESH_BINARY);
            int roi_x1 = std::max(0, cx - radius);
            int roi_y1 = std::max(0, cy - radius);
            int roi_x2 = std::min(rgbImage.cols, cx + (d - radius));
            int roi_y2 = std::min(rgbImage.rows, cy + (d - radius));
            cv::Mat roi = rgbImage(
                cv::Rect(cv::Point(roi_x1, roi_y1), cv::Point(roi_x2, roi_y2)));
            re.copyTo(roi, re_mask);
        } else {
            // first corner
            cv::line(
                rgbImage, Point(box.x0, box.y0),
                Point(box.x0 + (box.x1 - box.x0) / 4, box.y0),
                cv::Scalar(LabelColors[color_idx][2], LabelColors[color_idx][1],
                           LabelColors[color_idx][0]),
                3, 8, 0);
            cv::line(
                rgbImage, Point(box.x0, box.y0),
                Point(box.x0, box.y0 + (box.y1 - box.y0) / 4),
                cv::Scalar(LabelColors[color_idx][2], LabelColors[color_idx][1],
                           LabelColors[color_idx][0]),
                3, 8, 0);

            // second corner
            cv::line(
                rgbImage, Point(box.x0 + (box.x1 - box.x0) / 4 * 3, box.y0),
                Point(box.x0 + (box.x1 - box.x0), box.y0),
                cv::Scalar(LabelColors[color_idx][2], LabelColors[color_idx][1],
                           LabelColors[color_idx][0]),
                3, 8, 0);
            cv::line(
                rgbImage, Point(box.x0 + (box.x1 - box.x0), box.y0),
                Point(box.x0 + (box.x1 - box.x0),
                      box.y0 + (box.y1 - box.y0) / 4),
                cv::Scalar(LabelColors[color_idx][2], LabelColors[color_idx][1],
                           LabelColors[color_idx][0]),
                3, 8, 0);

            // third corner
            cv::line(
                rgbImage,
                Point(box.x0 + (box.x1 - box.x0),
                      box.y0 + (box.y1 - box.y0) / 4 * 3),
                Point(box.x0 + (box.x1 - box.x0), box.y0 + (box.y1 - box.y0)),
                cv::Scalar(LabelColors[color_idx][2], LabelColors[color_idx][1],
                           LabelColors[color_idx][0]),
                3, 8, 0);
            cv::line(
                rgbImage,
                Point(box.x0 + (box.x1 - box.x0), box.y0 + (box.y1 - box.y0)),
                Point(box.x0 + (box.x1 - box.x0) / 4 * 3,
                      box.y0 + (box.y1 - box.y0)),
                cv::Scalar(LabelColors[color_idx][2], LabelColors[color_idx][1],
                           LabelColors[color_idx][0]),
                3, 8, 0);

            // fourth corner
            cv::line(
                rgbImage, Point(box.x0, box.y0 + (box.y1 - box.y0)),
                Point(box.x0 + (box.x1 - box.x0) / 4,
                      box.y0 + (box.y1 - box.y0)),
                cv::Scalar(LabelColors[color_idx][2], LabelColors[color_idx][1],
                           LabelColors[color_idx][0]),
                3, 8, 0);
            cv::line(
                rgbImage, Point(box.x0, box.y0 + (box.y1 - box.y0)),
                Point(box.x0, box.y0 + (box.y1 - box.y0) / 4 * 3),
                cv::Scalar(LabelColors[color_idx][2], LabelColors[color_idx][1],
                           LabelColors[color_idx][0]),
                3, 8, 0);
        }
    }

    auto time_now = getCurrentTime();
    if (has_face && max_area > 6400) {
        last_hasface_time_ = time_now;
    } else {
        if (time_now - last_hasface_time_ > 2000) {
            std::string date, time;
            GetDateTime(date, time);

            // rgbImage = template_.clone()
            if (EndsWith(date, "Sat") || EndsWith(date, "Sun")) {
                rgbImage = template2_.clone();
                cv::putText(rgbImage, date, cv::Point(50, 200), 3, 1.5,
                            cv::Scalar(150, 150, 150), 1.5);
                cv::putText(rgbImage, time, cv::Point(40, 300), 1, 6,
                            cv::Scalar(231, 152, 35), 5);
            } else {
                rgbImage = template_.clone();
                // cv::putText(rgbImage, date, cv::Point(80, 200), 3, 1.5,
                // cv::Scalar(255, 255, 255), 1.5); cv::putText(rgbImage, time,
                // cv::Point(80, 300), 1, 6, cv::Scalar(255, 255, 255), 5);
                std::string greet;
                GetTimeGreetings(time, greet);
                cv::putText(rgbImage, greet, cv::Point(350, 130), 3, 1.2,
                            cv::Scalar(150, 150, 150), 1.8);
                cv::putText(rgbImage, date, cv::Point(50, 200), 3, 1.5,
                            cv::Scalar(150, 150, 150), 1.5);
                cv::putText(rgbImage, time, cv::Point(40, 300), 1, 6,
                            cv::Scalar(231, 152, 35), 5);
            }
            cv::flip(rgbImage, rgbImage, 1);
        }
    }
}

void DetectorProcessor::run() {
    QueuePtr stream_queue;
    QueueManager::SafeGet(WHALE_PROCESSOR_WEBSOCKETSTREAM, stream_queue);
    bool whale_door = ConfigureManager::instance()->getAsInt("whale_door");
    ServiceMetricManager::set(MetricAction::kAlgoModelMonitor,
                              "cv_output_frame_count", 0);
    ServiceMetricManager::set(MetricAction::kAlgoModelMonitor,
                              "cv_detect_count", 0);
    ServiceMetricManager::set(MetricAction::kAlgoModelMonitor, "cv_detect_cost",
                              0);

    while (true) {
        if (gt->sys_quit) {
            break;
        }

        boost::any context;
        if (input_queue_->Empty()) 
            continue;
        input_queue_->Pop(context);

        /*@note frame count, cv_output_frame_count*/
        ServiceMetricManager::add(MetricAction::kAlgoModelMonitor,
                                  "cv_output_frame_count", 1);

        std::shared_ptr<WhaleFrame> frame =
            boost::any_cast<std::shared_ptr<WhaleFrame>>(context);

        if (frame->cvImage.empty()) {
            LOG(INFO) << " queue size: " << input_queue_->Size();
            continue;
        }

        std::vector<Box> object_boxes;
        int64_t cur_time = getCurrentTime();
        whale::core::sdk::detect(frame, object_boxes);
        int64_t duration = getCurrentTime() - cur_time;

        ServiceMetricManager::add(MetricAction::kAlgoModelMonitor,
                                  "cv_detect_count", 1);
        ServiceMetricManager::add(MetricAction::kAlgoModelMonitor,
                                  "cv_detect_cost", duration);

        // image monitor
        int64_t diff = frame->camera_time - last_image_monitor_time_;
        if (diff > image_monitor_interval || diff <0) {
            cv::Mat cloned = frame->cvImage.clone();
            last_image_monitor_time_ = frame->camera_time;
            std::shared_ptr<Event> ent = std::make_shared<LoggerEvent>(
                frame->camera_sn, cloned, frame->camera_time,
                MetricAction::kImageStatusMonitor);
            output_queue_->Push(ent);
        }

        if (frame->camera_time - last_model_metric_monitor_time_ >
            WHALE_ALGO_MODEL_METRIC_INTERVAL) {
            last_model_metric_monitor_time_ = frame->camera_time;
            std::shared_ptr<Event> ent = std::make_shared<LoggerEvent>(
                frame->camera_sn, frame->camera_time,
                MetricAction::kAlgoModelMonitor);
            output_queue_->Push(ent);
        }

        auto data = std::make_shared<WhaleData>(object_boxes, frame);
        for (auto& kv : next_processor_unit_) kv.second->Push(data);
        // push  video stream.
        if (whale_door && gt->stream_connect) {
            cv::Mat stream_image = (frame->cvImage).clone();
            labelBox(stream_image, object_boxes);
            stream_queue->Push(stream_image, false);
        }
    }
    // process cell exit
    // delete self from pipelin
    LOG(INFO) << processor_name_ << " exit .......";
}

}  // namespace vision
}  // namespace whale
