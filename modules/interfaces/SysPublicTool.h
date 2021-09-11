#pragma once

#include <dirent.h>  //遍历系统指定目录下文件要包含的头文件
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>

#include <algorithm>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <queue>
#include <string>
#include <vector>

#include "AlgoData.h"

namespace whale {
namespace vision {
// todo select light-weight key-value db to store service configure.

class SysPublicTool {
 public:
    // 这个搜索函数 pattern 必须为正则表达式，不然崩溃
    static std::queue<std::string> searchFile(std::string path,
                                              std::string pattern);

    static void deleteFile(const std::string& file);
    static std::string loadConfigItem(const std::string& file,
                                      const std::string& str_default = "0");
    static void writeConfigItem(const std::string& file,
                                const std::string& item);

    static cv::Rect getBiggerRoi(cv::Rect& loc, cv::Mat& _img);
    static cv::Rect getBiggerRoi(cv::Rect& loc,
                                 std::shared_ptr<whale::core::BaseFrame> _img);
    static int getLocalIP(const char* eth_inf, char* ip);
    static int getRouteSn(std::string& route_sn);
    static int64_t getCurrentTime();
    static std::string getCurrentTimeString(bool bLocal = true);
    static const int64_t s_MicroSecondsPerSecond = 1000 * 1000;
    static void getFiles(std::string path, std::vector<std::string>& files,
                         std::string format = "");
    static void _mkdir(const char* dir);
    static std::vector<std::string> split(
        const std::string& s,
        const std::string& seperator);  //分割字符串
    static bool includeChinese(const char* str);
};
}  // namespace vision
}  // namespace whale