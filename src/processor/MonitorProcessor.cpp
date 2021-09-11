#include "processor/MonitorProcessor.h"

#include <algorithm>
#include <fstream>
#include <thread>

#include "event/Event.h"
#include "mgr/ConfigureManager.h"
#include "mgr/QueueManager.h"
#include "util.h"

using namespace whale::core;

namespace whale {
namespace vision {

void MonitorProcessor::init() {
    QueueManager::SafeGet(processor_name_, input_queue_);
    QueueManager::SafeGet(WHALE_PROCESSOR_DISPATCH, output_queue_);
    monitor_interval =
        ConfigureManager::instance()->getAsInt("monitor_interval");
    cur_interval_cnt_ = 0;

    cls_id_map = {{"cls_ped", 1}, {"cls_face", 2}, {"cls_head", 3}};

    // create tracker
    tracker_ = std::make_shared<Tracker>();
    tracker_->bbox_area_extract_feature_thresh = 1.0;  // block reid
    tracker_->iou_thresh = 0.3;
    tracker_->valid_leave_with_reid = false;

    need_visualize = false;
}

void MonitorProcessor::run() {
    while (true) {
        if (gt->sys_quit) {
            break;
        }
        boost::any context;
        if (input_queue_->Empty()) 
            continue;
        input_queue_->Pop(context);
        std::shared_ptr<WhaleData> message =
            boost::any_cast<std::shared_ptr<WhaleData>>(context);
        handle(message);
    }
    // process cell exit
    // delete self from pipeline
    LOG(INFO) << processor_name_ << " exit .......";
}

void MonitorProcessor::CvtDetToPed(vector<Box> &det_boxes,
                                   vector<PersonObject> &ped_det,
                                   unsigned long long timestamp,
                                   unsigned long long frame_id) {
    // only for person by now
    for (int i = 0; i < det_boxes.size(); ++i) {
        if (det_boxes[i].class_idx == cls_id_map["cls_ped"]) {
            BBoxMessage body_bbox_msg =
                ConvertBoxToBBoxMessage(det_boxes[i], timestamp, frame_id);
            ped_det.push_back(
                PersonObject(nullptr, &body_bbox_msg, timestamp, frame_id, -1));
        }
    }
}

void MonitorProcessor::handle(std::shared_ptr<WhaleData> msg) {
    std::vector<PersonObject> ped_leave, ped_det;
    std::vector<PersonObject> ped_tracked_tmp, ped_iou_dismatch_tmp,
        ped_expired_tmp;  // useless param bynow,change later
    // cvt det res 2 ped
    CvtDetToPed(msg->object_boxes, ped_det, msg->frame_->camera_time,
                msg->frame_->frame_id);

    // tracking
    {
        // TimingProfiler profile(processor_name_);
        tracker_->Update(msg->frame_->getcvMat(), ped_det,
                         msg->frame_->camera_time, msg->frame_->frame_id,
                         ped_tracked_tmp, ped_iou_dismatch_tmp, ped_expired_tmp,
                         ped_leave);
    }

    std::vector<PersonObject> &ped_tracked =
        tracker_->GetTrackedPersonObjects();

    cur_interval_cnt_++;
    if (cur_interval_cnt_ % monitor_interval == 0) {
        monitor(msg->frame_->camera_sn, msg->frame_->camera_time,
                msg->frame_->getcvMat(), ped_tracked, ped_leave);
        cur_interval_cnt_ = 0;
    }

    if (need_visualize) {
        cv::Mat img_show = msg->frame_->getcvMat().clone();
        visualize(msg->frame_->getcvMat(), img_show, ped_tracked);
    }
}

void MonitorProcessor::visualize(cv::Mat frame, cv::Mat &image_show,
                                 vector<PersonObject> &ped_tracked) {
    string tmp_text;
    cv::Scalar color = cv::Scalar(212, 255, 127);
    for (int i = 0; i < ped_tracked.size(); i++) {
        cv::Rect body_rect = ped_tracked[i].last_updated_body_bbox.bbox_rect;

        int id = ped_tracked[i].id;
        int x_ground = int(body_rect.x + body_rect.width / 2);
        int y_ground = int(body_rect.y + body_rect.height);

        cv::line(image_show, cv::Point(x_ground - 10, y_ground),
                 cv::Point(x_ground + 10, y_ground), cv::Scalar(0, 0, 255), 3);
        cv::line(image_show, cv::Point(x_ground, y_ground - 10),
                 cv::Point(x_ground, y_ground + 10), cv::Scalar(0, 0, 255), 3);

        tmp_text = "ID ";
        tmp_text += std::to_string(id);

        cv::rectangle(image_show, body_rect, color, 3);
        cv::rectangle(image_show, cv::Rect{body_rect.x, body_rect.y, 100, 50},
                      color, CV_FILLED);
        cv::putText(
            image_show, tmp_text, cv::Point(body_rect.x + 10, body_rect.y + 30),
            cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);
    }

    //    video_writer_->write(image_show);
    cv::namedWindow("frame", cv::WINDOW_NORMAL);
    cv::imshow("frame", image_show);
    int key = cv::waitKey(33);
    if (key == 113) {
        exit(0);
    }
}

void MonitorProcessor::monitor(const std::string &camera_sn,
                               int64_t camera_time, cv::Mat frame,
                               vector<PersonObject> &ped_tracked,
                               vector<PersonObject> &ped_leave) {
    for (int i = 0; i < ped_tracked.size(); ++i) {
        if (ped_tracked[i].object_status == confirmed &&
            ped_tracked[i].is_current_body_detected) {
            cv::Rect body_rect =
                ped_tracked[i].last_updated_body_bbox.bbox_rect;
            int id = ped_tracked[i].id;
            int x_ground = int(body_rect.x + body_rect.width / 2);
            int y_ground = int(body_rect.y + body_rect.height);
            x_.push_back(x_ground);
            y_.push_back(y_ground);
            id_.push_back(id);
        }
    }
    if (x_.size() > 0) {
        std::shared_ptr<Event> pae =
            std::make_shared<MonitorEvent>(camera_sn, x_, y_, id_, camera_time);

        LOG(INFO) << "push Monitor event.";
        output_queue_->Push(pae);
        x_.clear();
        y_.clear();
        id_.clear();
    }
};

}  // namespace vision
}  // namespace whale
