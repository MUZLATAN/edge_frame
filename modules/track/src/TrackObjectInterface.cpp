//
// Created by test on 2019/12/23.
//

#include "TrackObjectInterface.h"

#include "AlgoData.h"

namespace whale {
namespace vision {

bool IsFaceAndBodyMatch(Box &face, Box &body) {
//    assert(face.class_idx == cls_face && body.class_idx == cls_ped);
    Rect_<float> face_rect(face.x0, face.y0, face.x1 - face.x0,
                           face.y1 - face.y0);
    Rect_<float> body_rect(body.x0, body.y0, body.x1 - body.x0,
                           body.y1 - body.y0);
    return 1.0 * (face_rect & body_rect).area() / face_rect.area() > 0.8;
}

BBoxMessage ConvertBoxToBBoxMessage(Box &object_box,
                                    unsigned long long update_timestamp,
                                    unsigned long long update_frame_id) {
    return BBoxMessage{
        .update_timestamp = update_timestamp,
        .update_frame_id = update_frame_id,
        .score = object_box.score,
        .bbox_rect = cv::Rect_<float>(object_box.x0, object_box.y0,
                                      object_box.x1 - object_box.x0,
                                      object_box.y1 - object_box.y0)};
}

void MatchCurDetectFaceAndBody(vector<Box> &object_boxes,
                               vector<PersonObject> &cur_detect_person_objects,
                               float face_score_thresh, float face_area_thresh,
                               unsigned long long update_timestamp,
                               unsigned long long update_frame_id) {
    int id = 0;
    vector<bool> match_table = vector<bool>(object_boxes.size());
    for (int i = 0; i < match_table.size(); i++) {
        match_table[i] = false;
    }
    for (int i = 0; i < object_boxes.size(); i++) {
        if (object_boxes[i].class_idx == cls_face && !match_table[i]) {
            float face_area = (object_boxes[i].x1 - object_boxes[i].x0) *
                              (object_boxes[i].y1 - object_boxes[i].y0);
            if (face_area < face_area_thresh ||
                object_boxes[i].score < face_score_thresh)
                continue;
            for (int j = 0; j < object_boxes.size(); j++) {
                if (object_boxes[j].class_idx == cls_ped && !match_table[j]) {
                    if (IsFaceAndBodyMatch(object_boxes[i], object_boxes[j])) {
                        match_table[i] = true;
                        match_table[j] = true;
                        BBoxMessage face_bbox_message = ConvertBoxToBBoxMessage(
                            object_boxes[i], update_timestamp, update_frame_id);
                        BBoxMessage body_bbox_message = ConvertBoxToBBoxMessage(
                            object_boxes[j], update_timestamp, update_frame_id);
                        cur_detect_person_objects.push_back(PersonObject(
                            &face_bbox_message, &body_bbox_message,
                            update_timestamp, update_frame_id, -1));
                    }
                }
            }
        }
    }
    for (int i = 0; i < object_boxes.size(); i++) {
        if (!match_table[i]) {
            if (object_boxes[i].class_idx == cls_ped) {
                BBoxMessage body_bbox_message = ConvertBoxToBBoxMessage(
                    object_boxes[i], update_timestamp, update_frame_id);
                cur_detect_person_objects.push_back(
                    PersonObject(nullptr, &body_bbox_message, update_timestamp,
                                 update_frame_id, -1));
            } else if (object_boxes[i].class_idx == cls_face) {
                // preserved, add unmatched face bbox as a person object,
                // without body bbox
                //                BBoxMessage face_bbox_message =
                //                ConvertBoxToBBoxMessage(object_boxes[i],
                //                update_timestamp, update_frame_id);
                //                cur_detect_person_objects.push_back(PersonObject(&face_bbox_message,
                //                nullptr, update_timestamp, update_frame_id,
                //                -1));
            }
        }
    }
}

void MatchCurDetectFaceAndBody(std::map<std::string, int> &cls_id_map,
                               vector<Box> &object_boxes,
                               vector<PersonObject> &cur_detect_person_objects,
                               float face_area_thresh,
                               unsigned long long update_timestamp,
                               unsigned long long update_frame_id) {
    int cls_face = cls_id_map["cls_face"];
    int cls_head = cls_id_map["cls_head"];
    int cls_ped = cls_id_map["cls_ped"];
    int id = 0;
    vector<bool> match_table = vector<bool>(object_boxes.size());
    for (int i = 0; i < match_table.size(); i++) {
        match_table[i] = false;
    }
    for (int i = 0; i < object_boxes.size(); i++) {
        if (object_boxes[i].class_idx == cls_face && !match_table[i]) {
            float face_area = (object_boxes[i].x1 - object_boxes[i].x0) *
                              (object_boxes[i].y1 - object_boxes[i].y0);
            if (face_area < face_area_thresh) continue;
            for (int j = 0; j < object_boxes.size(); j++) {
                if (object_boxes[j].class_idx == cls_ped && !match_table[j]) {
                    if (IsFaceAndBodyMatch(object_boxes[i], object_boxes[j])) {
                        match_table[i] = true;
                        match_table[j] = true;
                        BBoxMessage face_bbox_message = ConvertBoxToBBoxMessage(
                            object_boxes[i], update_timestamp, update_frame_id);
                        BBoxMessage body_bbox_message = ConvertBoxToBBoxMessage(
                            object_boxes[j], update_timestamp, update_frame_id);
                        cur_detect_person_objects.push_back(PersonObject(
                            &face_bbox_message, &body_bbox_message,
                            update_timestamp, update_frame_id, -1));
                    }
                }
            }
        }
    }
    for (int i = 0; i < object_boxes.size(); i++) {
        if (!match_table[i]) {
            if (object_boxes[i].class_idx == cls_ped) {
                BBoxMessage body_bbox_message = ConvertBoxToBBoxMessage(
                    object_boxes[i], update_timestamp, update_frame_id);
                cur_detect_person_objects.push_back(
                    PersonObject(nullptr, &body_bbox_message, update_timestamp,
                                 update_frame_id, -1));
            } else if (object_boxes[i].class_idx == cls_face) {
            }
        }
    }
}

}  // namespace vision
}  // namespace whale