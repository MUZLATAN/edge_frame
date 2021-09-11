#pragma once

#include <json/json.h>
#include <glog/logging.h>

#include <boost/signals2.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace whale {
namespace vision {

enum class CONFIGType {
    DEFAULT,
    VIDEORECORD,
    _END_,  // NOTE: jinpengli： 这个值不能被显式使用
};

class ConfigureManager {
 public:
    ConfigureManager();
    ConfigureManager(const ConfigureManager&);
    ~ConfigureManager(){};

    double getAsDouble(const std::string& key);
    int getAsInt(const std::string& key);
    Json::Value getAsAarry(const std::string& key);
    std::string getAsString(const std::string& key);
    bool check(const std::string& key);
    std::vector<std::string> getAsVectorString(const std::string& key);
    boost::signals2::signal<void()> toUpdated;

    void setAsInt(const std::string& key, int value);

    static ConfigureManager* instance(CONFIGType type = CONFIGType::DEFAULT);

    void init();

 private:
    ConfigureManager& operator=(const ConfigureManager&);

 private:
    Json::Value config_option_ ;
    std::string config_path;
};

}  // namespace vision
}  // namespace whale