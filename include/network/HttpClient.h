#pragma once

#include <curl/curl.h>


#include <string>
namespace whale {
namespace vision {

bool HttpGet(const char *url, const std::string &value, std::string &re);

bool HttpPost(const char *url, const std::string &value, std::string &re);

bool IOTHeartBeat(const std::string &device_sn);



#ifdef __BUILD_GTESTS__
std::string LoadLastHttpSendData(void);
#endif

}  // namespace vision
}  // namespace whale