#include <dirent.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <chrono>
#include <opencv2/opencv.hpp>
#include <thread>

#include "ServiceMetricManager.h"
#include "SysPublicTool.h"
#include "mgr/ConfigureManager.h"
#include "mgr/QueueManager.h"
#include "processor/DataLoaderProcessor.h"

namespace whale {
namespace vision {

void UsbDeviceLoaderProcessor::init() {
    QueueManager::SafeGet(WHALE_PROCESSOR_DETECT, output_queue_);
    LOG(INFO) << "usb test get value: " << gt->camera_sns;

    // init processor
    int cam_id = findCamera();
    LOG(INFO) << " camera id: " << cam_id;
    capture_ = std::make_unique<cv::VideoCapture>(cam_id);
    capture_->set(cv::CAP_PROP_BUFFERSIZE, 3);

    capture_->set(cv::CAP_PROP_FPS, 15);
    capture_->set(CV_CAP_PROP_FOURCC, CV_FOURCC('M', 'J', 'P', 'G'));

    if (cam_id >= 0) {
        selectResolution();
        capture_->set(cv::CAP_PROP_FRAME_WIDTH, width_);
        capture_->set(cv::CAP_PROP_FRAME_HEIGHT, height_);
    }

    if (capture_->isOpened()) {
        cv::Mat frame;
        if (capture_->read(frame)) {
            ConfigureManager::instance()->setAsInt("video_width", frame.cols);
            ConfigureManager::instance()->setAsInt("video_height", frame.rows);
            if (width_ != frame.cols || height_ != frame.rows) {
                LOG(ERROR) << "select resolution: [" << width_ << "," << height_
                           << "], after set,  get resolution:[ " << frame.cols
                           << "," << frame.rows << " ]";
            }
            LOG(INFO)
                << "We set the video_width/video_height to ConfigureManager.";
        }
    } else {
        reopenCamera();
    }

    image_rotate_angle_ =
        ConfigureManager::instance()->getAsInt("image_rotate_angle");
}

void UsbDeviceLoaderProcessor::rotateImage(cv::Mat& src_img, int degree,
                                           cv::Mat& des_img) {
    switch (degree) {
        case 90: {
            cv::Mat img_transpose;
            cv::transpose(src_img, img_transpose);
            cv::flip(img_transpose, des_img, 1);
            break;
        }
        case 180: {
            cv::flip(src_img, des_img, -1);
            break;
        }
        case 270: {
            cv::Mat img_transpose;
            cv::transpose(src_img, img_transpose);
            cv::flip(img_transpose, des_img, 0);
            break;
        }
        default: {
            printf("The input degree is invalid. \n");
            des_img = src_img;
            break;
        }
    }
}

void UsbDeviceLoaderProcessor::selectResolution() {
    std::vector<std::pair<int, int>> candidate_resolution;
    bool selected = false;

    // first select 960, 540
    for (size_t i = 0; i < suppered_resolution_vec_.size(); ++i) {
        if (suppered_resolution_vec_[i].first == 960) {
            candidate_resolution.push_back(suppered_resolution_vec_[i]);
            if (suppered_resolution_vec_[i].second == 960 * 9 / 16) {
                width_ = suppered_resolution_vec_[i].first;
                height_ = suppered_resolution_vec_[i].second;
                selected = true;
            }
        }
    }

    if (!selected) {
        if (candidate_resolution.size() > 0) {
            // simple select
            width_ = candidate_resolution[0].first;
            height_ = candidate_resolution[0].second;
        } else {
            int idx = -1;
            int min_diff = 99999;
            for (size_t i = 0; i < suppered_resolution_vec_.size(); ++i) {
                int diff = abs(960 - suppered_resolution_vec_[i].first);
                if (diff < min_diff) {
                    min_diff = diff;
                    idx = i;
                }
            }

            width_ = suppered_resolution_vec_[idx].first;
            height_ = suppered_resolution_vec_[idx].second;
        }
    }

    LOG(INFO) << "cam real width:" << width_ << " ,real height:" << height_;
}

int UsbDeviceLoaderProcessor::findCamera() {
    DIR* dir = opendir("/dev");
    struct dirent* entry;
    int ret;
    bool flag = false;
    if (dir != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            ret = fnmatch("video*", entry->d_name, FNM_PATHNAME | FNM_PERIOD);
            if (ret == 0) {
                flag = true;
                int video = entry->d_name[5] - '0';
                // LOG(INFO) << entry->d_name[5] << " " << video ;
                if (infoCamera("/dev/video" + std::to_string(video))) {
                    closedir(dir);
                    return video;
                }
            } else if (ret == FNM_NOMATCH)
                continue;
        }
    }
    closedir(dir);
    return -1;
}

void UsbDeviceLoaderProcessor::reopenCamera() {
    while (true) {
        is_ready_ = false;
        if (capture_) {
            capture_->release();
            capture_ = std::make_unique<cv::VideoCapture>(-1);
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));
        LOG(INFO) << "******Trying to reconnect camera now****";

        int cam_id = findCamera();
        LOG(INFO) << "camera id: " << cam_id;

        if (cam_id < 0) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            LOG(INFO) << " search camera id is invalid. ";
            continue;
        }

        selectResolution();

        bool opened = capture_->open(cam_id);
        if (!capture_->isOpened()) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            LOG(INFO) << " image frame isOpened is false. opened flag: "
                      << opened;
            continue;
        }

        capture_->set(cv::CAP_PROP_FRAME_WIDTH, width_);
        capture_->set(cv::CAP_PROP_FRAME_HEIGHT, height_);
        capture_->set(cv::CAP_PROP_FPS, 15);
        capture_->set(CV_CAP_PROP_FOURCC, CV_FOURCC('M', 'J', 'P', 'G'));

        LOG(INFO) << " camid: " << cam_id << "width: " << width_
                  << " height: " << height_;

        // set camera video param
        cv::Mat frame;
        if (!capture_->read(frame)) {
            LOG(INFO) << " image frame empty. ";
            continue;
        }

        ConfigureManager::instance()->setAsInt("video_width", frame.cols);
        ConfigureManager::instance()->setAsInt("video_height", frame.rows);
        LOG(INFO) << "opened: " << frame.cols << " " << frame.rows;
        if (width_ != frame.cols || height_ == frame.rows) {
            LOG(ERROR) << "select resolution: [" << width_ << "," << height_
                       << "], after set resolution:[ " << frame.cols << ","
                       << frame.rows << " ]";
        }

        frame_id_ = 0;
        return;
    }
}

void UsbDeviceLoaderProcessor::run() {
    QueuePtr stream_queue;
    QueueManager::SafeGet(WHALE_PROCESSOR_WEBSOCKETSTREAM, stream_queue);
    // std::string camera_sn = ServiceVariableManager::Get(usb_device_);
    std::string camera_sn = gt->camera_sns;
    bool whale_door = ConfigureManager::instance()->getAsInt("whale_door");
    int64_t last_heartbeat_time = 0;

    LOG(INFO) << "---->START: " << __FUNCTION__;

    while (true) {
        if (gt->sys_quit) {
            return;
        }

        auto frame = std::make_shared<whale::core::WhaleFrame>();
        if (!capture_->read(frame->cvImage)) {
            // while loop for lookfor camera device
            LOG(INFO) << " reopen camera.";
            reopenCamera();
            continue;
        }
        // TODO: this place need to test mem addr
        frame->cvImage = frame->cvImage.clone();

        if (image_rotate_angle_ > 0) {
            rotateImage(frame->cvImage, image_rotate_angle_, frame->cvImage);
        }

        frame_id_++;
        frame->setCurrentTime();
        frame->camera_sn = camera_sn;
        frame->frame_id = frame_id_;
        output_queue_->Push(frame, false);

        if (frame->camera_time - last_heartbeat_time >
            WHALE_HEARTBEAT_INTERVAL) {
            IOTHeartBeat(gt->camera_sns);
            last_heartbeat_time = frame->camera_time;
        }

        /*@note in frame count, cv_input_frame_count*/
        ServiceMetricManager::add(MetricAction::kAlgoModelMonitor,
                                  "cv_input_frame_count", 1);

        is_ready_ = true;
        if (!whale_door && gt->stream_connect) {
            cv::Mat stream_image = (frame->cvImage).clone();
            stream_queue->Push(stream_image, false);
        }

        videoRe(frame);
    }
}

bool UsbDeviceLoaderProcessor::infoCamera(const std::string& devName) {
    if (devName.empty()) return false;

    int fd, i, j, k;
    fd = open(devName.c_str(), O_RDWR);
    if (fd < 0) {
        perror("open dev");
        return false;
    }

    //这些设备文件是视频相关驱动产生的，摄像头只是视频设备中的一种，除此外还有硬件编解码的设备，图形加速等高设备。
    //所以/dev/video?设备文件不一定是摄像头相关的设备文件.

    //检查打开的设备文件是不是摄像头的设备文件.
    struct v4l2_capability cap;
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
        close(fd);
        perror("query cap");
        return false;
    }
    // uvcvideo是linux下通用的usb摄像头驱动 	printf("card: %s\n", cap.card);
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        close(fd);
        return false;
    }
    //查询此设备文件对应的摄像头驱动支持的摄像头个数
    struct v4l2_input inp;
    int inpNum = 0;
    for (i = 0;; i++) {
        inp.index = i;
        if (ioctl(fd, VIDIOC_ENUMINPUT, &inp) < 0) break;
        inpNum++;
    }
    if (inpNum <= 0) return false;

    //如果支持多个摄像头，可通过ioctl的VIDIOC_S_INPUT指定使用第几个摄像头
    //查询摄像头支持的图像数据格式
    struct v4l2_fmtdesc fmtd;  //存的是摄像头支持的传输格式
    struct v4l2_frmsizeenum
        frmsize;  //存的是摄像头，对应的图片格式所支持的分辨率
    struct v4l2_frmivalenum
        framival;  //存的时，对应的图片格式，分辨率所支持的帧率
    int fmtdNum = 0;
    int frmsizeNum = 0;
    int framivalNum = 0;
    for (i = 0;; i++) {
        fmtd.index = i;
        fmtd.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd, VIDIOC_ENUM_FMT, &fmtd) < 0) break;
        fmtdNum++;
        // 查询这种图像数据格式下支持的分辨率
        for (j = 0;; j++) {
            frmsize.index = j;
            frmsize.pixel_format = fmtd.pixelformat;
            // fmtd.pixelformat 四字符代码（不知道是什么鬼，总之是识别图片格式）
            if (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) < 0) break;
            frmsizeNum++;

            std::pair<int, int> resolution =
                std::make_pair(frmsize.discrete.width, frmsize.discrete.height);
            suppered_resolution_vec_.emplace_back(resolution);
            // LOG(INFO) << "suppered resolution: " << frmsize.discrete.width
            //           << " " << frmsize.discrete.height;
            // frame rate
            for (k = 0;; k++) {
                framival.index = k;
                framival.pixel_format = fmtd.pixelformat;
                framival.width = frmsize.discrete.width;
                framival.height = frmsize.discrete.height;
                if (ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &framival) < 0) break;
                framivalNum++;
            }
        }
    }

    // close fd, if don't close fd, it will lead to opencv open error.
    close(fd);

    if (fmtdNum > 0 && frmsizeNum > 0 && framivalNum > 0) {
        return true;
    } else {
        return false;
    }
}

}  // namespace vision
}  // namespace whale
