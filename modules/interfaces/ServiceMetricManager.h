#pragma once
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

namespace whale {
namespace vision {

enum class MetricAction {
    kServiceVersionMonitor = 1,
    kAlgoModelMonitor,
    kAlgoDataMonitor,
    kFlowStatusMonitor,
    kStreamStatusMonitor,
    kImageStatusMonitor,
    kApiStatusMonitor,
    __END__,  // NOTE: reserved value,cannot use this!!!
};

class ServiceMetricManager {
 public:
    ServiceMetricManager() {}
    ~ServiceMetricManager() {}

    static std::unordered_map<std::string, int> getAndClear(
        MetricAction action);
    static std::unordered_map<std::string, int> get(MetricAction action);

    static void add(MetricAction action, const std::string& key, int value);
    static void set(MetricAction action, const std::string& key, int value);
};

}  // namespace vision
}  // namespace whale