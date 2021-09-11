#include "util.h"
namespace whale {
namespace vision {

// the feature pipeline
std::map<std::string, std::vector<std::string>> feature_map = {
		{"monitor", {"MonitorProcessor"}}
		};
}	 // namespace vision
}	 // namespace whale
