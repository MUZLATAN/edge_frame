#include "SysPublicTool.h"

#include <arpa/inet.h>
#include <dirent.h>
#include <glog/logging.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <algorithm>
#include <boost/filesystem.hpp>
#include <chrono>
#include <fstream>
#include <regex>
#include <thread>


namespace whale {
namespace vision {

std::queue<std::string> SysPublicTool::searchFile(std::string path,
                                                  std::string pattern) {
    std::queue<std::string> file_queue;
    try {
        std::regex pattern_regex(pattern);
        DIR* dir = opendir(path.c_str());
        struct dirent* entry;
        if (dir != NULL) {
            while ((entry = readdir(dir)) != NULL) {
                if (std::regex_match(std::string(entry->d_name),
                                     pattern_regex)) {
                    file_queue.push(entry->d_name);
                    LOG(INFO) << " search match file: " << entry->d_name;
                }
            }
        }

        closedir(dir);
        return file_queue;
    } catch (const std::exception& e) {
        LOG(INFO) << e.what();
        return file_queue;
    }
}

void SysPublicTool::deleteFile(const std::string& file) {
    remove(file.c_str());
}

// void SysPublicTool::uuidGenerate(std::string& uuid) {
//     // #ifndef __PLATFORM_HISI__
//     //     uuid_t uu;
//     //     char buf[256] = {};
//     //     uuid_generate_random(uu);
//     //     uuid_unparse(uu, buf);
//     //     uuid = std::string(buf);
//     // #endif
//     boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
//     uuid = boost::uuids::to_string(a_uuid);
// }

void SysPublicTool::writeConfigItem(const std::string& file,
                                    const std::string& item) {
    std::ofstream out_file_stream;
    out_file_stream.open(file, std::ofstream::out | std::ofstream::trunc);
    out_file_stream << item;
    out_file_stream.close();
}

std::string SysPublicTool::loadConfigItem(const std::string& file,
                                          const std::string& str_default) {
    std::ifstream file_stream(file);
    std::string config_item;
    if (!file_stream) {
        std::ofstream out_file_stream(file);
        config_item = str_default;
        out_file_stream << config_item;
        out_file_stream.close();

    } else {
        file_stream >> config_item;
        file_stream.close();
    }

    if (config_item.empty()) {
        config_item = str_default;
    }
    return config_item;
}

// other tools function

cv::Rect SysPublicTool::getBiggerRoi(cv::Rect& loc, cv::Mat& _img) {
    cv::Rect big_roi;
    big_roi.x = std::max(loc.x - loc.width / 2, 0);
    big_roi.y = std::max(loc.y - loc.height / 2, 0);
    big_roi.width = std::min(2 * loc.width, _img.cols - big_roi.x);
    big_roi.height = std::min(2 * loc.height, _img.rows - big_roi.y);
    return big_roi;
}

cv::Rect SysPublicTool::getBiggerRoi(
    cv::Rect& loc, std::shared_ptr<whale::core::BaseFrame> _img) {
    cv::Rect big_roi;
    big_roi.x = std::max(loc.x - loc.width / 2, 0);
    big_roi.y = std::max(loc.y - loc.height / 2, 0);
    big_roi.width = std::min(2 * loc.width, _img->col() - big_roi.x);
    big_roi.height = std::min(2 * loc.height, _img->row() - big_roi.y);
    return big_roi;
}

int SysPublicTool::getLocalIP(const char* eth_inf, char* ip) {
    int sd;
    struct sockaddr_in sin;
    struct ifreq ifr;

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == sd) {
        LOG(INFO) << "socket error: " << strerror(errno);
        return -1;
    }

    strncpy(ifr.ifr_name, eth_inf, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;

    // if error: No such device
    if (ioctl(sd, SIOCGIFADDR, &ifr) < 0) {
        LOG(INFO) << "ioctl SIOCGIFADDR error: " << strerror(errno);
        close(sd);
        return -1;
    }

    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
    snprintf(ip, 16, "%s", inet_ntoa(sin.sin_addr));

    close(sd);
    return 0;
}

int SysPublicTool::getRouteSn(std::string& route_sn) {
#ifdef __PLATFORM_X86__
    route_sn = "WRA2019283FF1EA8F37";
    return 0;
#elif defined __PLATFORM_HISI__
	route_sn = loadConfigItem("/mnt/whale/sn", "");
	return 0;
#endif


    return -1;
}

int64_t SysPublicTool::getCurrentTime() {
    return std::chrono::system_clock::now().time_since_epoch() /
           std::chrono::milliseconds(1);
}

std::string SysPublicTool::getCurrentTimeString(bool bLocal) {
    struct timeval tvValue;
    gettimeofday(&tvValue, NULL);
    int64_t microSeconds = static_cast<int64_t>(
        tvValue.tv_sec * s_MicroSecondsPerSecond + tvValue.tv_usec);
    char arrBuffer[64] = {0};
    // gmtime()能够把日历时间转换为一个对应于UTC的分解时间，gmtime_r()是它的可重入版
    //原型：struct tm *gmtime_r(const time_t *timep, struct tm *result);
    // localtime()需要考虑时区和夏令时设置，localtime_r()是它的可重入版本
    //原型：struct tm *localtime_r(const time_t *timep, struct tm *result);
    struct tm tm_time;
    time_t second_time =
        static_cast<time_t>(microSeconds / s_MicroSecondsPerSecond);
    if (bLocal)
        localtime_r(&second_time, &tm_time);
    else
        gmtime_r(&second_time, &tm_time);

    snprintf(arrBuffer, sizeof(arrBuffer), "%04d-%02d-%02d-%02d-%02d-%02d",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    return arrBuffer;
}

void SysPublicTool::getFiles(std::string _path, std::vector<std::string>& files,
                             std::string format) {
    _path += "/";
    boost::filesystem::path p(_path);
    std::vector<boost::filesystem::path> v;
    copy(boost::filesystem::directory_iterator(p),
         boost::filesystem::directory_iterator(), back_inserter(v));

    sort(v.begin(), v.end());  // sort, since directory iteration
                               // is not ordered on some file systems
    for (auto& i : v)
        if (!is_directory(i)) {  // we eliminate directories
            std::string filename = i.filename().string();
            if (filename.find(format) == std::string::npos) continue;
            files.push_back(_path + filename);
        }
}

void SysPublicTool::_mkdir(const char* dir) {
    char tmp[256];
    char* p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/') tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++)
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, ACCESSPERMS);
            *p = '/';
        }
    mkdir(tmp, ACCESSPERMS);
}

std::vector<std::string> SysPublicTool::split(const std::string& s,
                                              const std::string& seperator) {
    std::vector<std::string> result;
    typedef std::string::size_type string_size;
    string_size i = 0;

    while (i != s.size()) {
        //找到字符串中首个不等于分隔符的字母；
        int flag = 0;
        while (i != s.size() && flag == 0) {
            flag = 1;
            for (string_size x = 0; x < seperator.size(); ++x)
                if (s[i] == seperator[x]) {
                    ++i;
                    flag = 0;
                    break;
                }
        }

        //找到又一个分隔符，将两个分隔符之间的字符串取出；
        flag = 0;
        string_size j = i;
        while (j != s.size() && flag == 0) {
            for (string_size x = 0; x < seperator.size(); ++x)
                if (s[j] == seperator[x]) {
                    flag = 1;
                    break;
                }
            if (flag == 0) ++j;
        }
        if (i != j) {
            result.push_back(s.substr(i, j - i));
            i = j;
        }
    }
    return result;
}

bool SysPublicTool::includeChinese(const char* str) {
    char c;
    while (1) {
        c = *str++;
        if (c == 0) break;  //如果到字符串尾则说明该字符串没有中文字符
        if (c & 0x80)  //如果字符高位为1且下一字符高位也是1则有中文字符
            if (*str & 0x80) return true;
    }
    return false;
}

}  // namespace vision
}  // namespace whale