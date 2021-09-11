//
// Created by test on 2019/12/23.
//

#ifndef FMS_CONVERT_DETECT_TO_TRACK_OBJECT_H
#define FMS_CONVERT_DETECT_TO_TRACK_OBJECT_H

#include "AlgoData.h"
#include "tracker.h"

using namespace whale::core;

namespace whale {
namespace vision {

bool IsFaceAndBodyMatch(Box &face, Box &body);

void MatchCurDetectFaceAndBody(vector<Box> &object_boxes,
                               vector<PersonObject> &cur_detect_person_objects,
                               float face_score_thresh, float face_area_thresh,
                               unsigned long long update_timestamp,
                               unsigned long long update_frame_id);
void MatchCurDetectFaceAndBody(std::map<std::string, int> &cls_id_map,
                               vector<Box> &object_boxes,
                               vector<PersonObject> &cur_detect_person_objects,
                               float face_area_thresh,
                               unsigned long long update_timestamp,
                               unsigned long long update_frame_id);
BBoxMessage ConvertBoxToBBoxMessage(Box &object_box,
                                    unsigned long long update_timestamp,
                                    unsigned long long update_frame_id);
}  // namespace vision
}  // namespace whale

#endif  // FMS_CONVERT_DETECT_TO_TRACK_OBJECT_H
