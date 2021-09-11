
#include "ServiceMetricManager.h"

#include <iostream>

namespace whale {
namespace vision {

std::mutex global_mt;

std::unordered_map<std::string, int> metrics_[(int)MetricAction::__END__];

void ServiceMetricManager::set(MetricAction action, const std::string& key,
                               int value) {
    std::lock_guard<std::mutex> lock(global_mt);
    metrics_[(int)action][key] = value;
}

std::unordered_map<std::string, int> ServiceMetricManager::getAndClear(
    MetricAction action) {
    std::lock_guard<std::mutex> lock(global_mt);
    std::unordered_map<std::string, int> ret;

    ret = metrics_[(int)action];
    // for (auto iter = metrics_[(int)action].begin(); iter !=
    // metrics_[(int)action].end();
    //      ++iter)
    //     iter->second = 0;
    metrics_[(int)action].clear();

    return ret;
}

std::unordered_map<std::string, int> ServiceMetricManager::get(
    MetricAction action) {
    std::lock_guard<std::mutex> lock(global_mt);
    return metrics_[(int)action];
}

void ServiceMetricManager::add(MetricAction action, const std::string& key,
                               int value) {
    std::lock_guard<std::mutex> lock(global_mt);
    metrics_[(int)action][key] += value;
}

}  // namespace vision
}  // namespace whale