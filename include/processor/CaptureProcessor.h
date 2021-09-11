
#pragma once
#include <glog/logging.h>

#include <map>
#include <opencv2/opencv.hpp>

#include "DynamicFactory.h"
#include "FaceSelector.h"
#include "FaceSelectorPad.h"
#include "PolygonUtils.h"
#include "Processor.h"
#include "SysPublicTool.h"
#include "TrackObjectInterface.h"
#include "interface.h"
#include "mgr/ConfigureManager.h"
#include "tracker.h"

namespace whale {
namespace vision {

struct Line {
    cv::Point pStart;
    cv::Point pEnd;
};


}  // namespace vision
}  // namespace whale
