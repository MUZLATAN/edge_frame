#pragma once
#include <mutex>
#include <opencv2/videoio.hpp>

#include "DynamicFactory.h"
#include "VideoRecorder.h"
#include "common.h"
#include "network/HttpClient.h"

namespace whale {
namespace vision {
class DataLoaderProcessor
    : public Processor,
      DynamicCreator<DataLoaderProcessor, const std::string&> {
 public:
    DataLoaderProcessor(const std::string& name) : Processor(name) {}
    virtual ~DataLoaderProcessor() {}
    virtual void init() {}
    virtual void run() {}
    void updateConf(){};
    void updateVideoRecordConf();
    // video record
    void videoRe(std::shared_ptr<whale::core::WhaleFrame> frame);

 public:
    std::mutex mt_;
    int recordNum = 1;
    std::shared_ptr<RGBVideoRecorder> recorder_;
};

class UsbDeviceLoaderProcessor
    : public DataLoaderProcessor,
      DynamicCreator<UsbDeviceLoaderProcessor, std::string&> {
 public:
    UsbDeviceLoaderProcessor(std::string& usb_device)
        : DataLoaderProcessor(WHALE_PROCESSOR_USBCAMERA),
          usb_device_(usb_device),
          is_ready_(false),
          frame_id_(0),
          global_frame_id_(0) {}
    virtual ~UsbDeviceLoaderProcessor() {}
    virtual void init();
    virtual void run();

 private:
    int findCamera();
    void reopenCamera();
    void selectResolution();
    bool infoCamera(const std::string& devName = "/dev/video0");
    void rotateImage(cv::Mat& src_img, int degree, cv::Mat& des_img);

 private:
    std::string usb_device_;
    bool is_ready_;
    std::unique_ptr<cv::VideoCapture> capture_;
    std::vector<std::pair<int, int>> suppered_resolution_vec_;
    int64_t frame_id_ = 0;
    int64_t global_frame_id_ = 0;
    int height_;
    int width_;
    int image_rotate_angle_ = 0;
};

class RtspLoaderProcessor : public DataLoaderProcessor,
                            DynamicCreator<RtspLoaderProcessor, std::string&> {
 public:
    RtspLoaderProcessor(std::string& rtsp)
        : DataLoaderProcessor(WHALE_PROCESSOR_RTSP), rtsp_stream_addr_(rtsp) {}
    virtual ~RtspLoaderProcessor() {}
    virtual void init();
    virtual void run();

 private:
    void reopenCamera();

 private:
    std::string rtsp_stream_addr_;
    bool is_ready_;
    std::unique_ptr<cv::VideoCapture> capture_;
    int64_t frame_id_ = 0;
};

class LocalVideoLoaderProcessor
    : public DataLoaderProcessor,
      DynamicCreator<LocalVideoLoaderProcessor, std::string&> {
 public:
    LocalVideoLoaderProcessor(std::string& local_video_file)
        : DataLoaderProcessor(WHALE_PROCESSOR_LOCALVIDEO),
          local_video_file_(local_video_file),
          is_ready_(false) {}
    virtual ~LocalVideoLoaderProcessor() {}
    void init();
    void run();

 private:
    std::string local_video_file_;  //当次加载的文件
    std::string local_csv_dir_;     // csv生成路径
    bool is_ready_;
    std::unique_ptr<cv::VideoCapture> capture_;
    std::vector<std::string> localVideoList_;
    int64_t frame_id_ = 0;
    int loadVideo();
    void getLocalRestVideo(std::string path);
};

}  // namespace vision
}  // namespace whale
