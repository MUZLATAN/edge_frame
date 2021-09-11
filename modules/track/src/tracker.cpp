//
// Created by neptune on 19-9-2.
//

#include "tracker.h"

#include <glog/logging.h>

#include "Hungarian.h"
#include "ServiceMetricManager.h"
#include "SysPublicTool.h"
#include "interface.h"

#define MAX_DIST 1.0
#define DEBUG_PRINT() printf("%s %s %d \n", __FILE__, __FUNCTION__, __LINE__);
namespace whale {
namespace vision {
void Tracker::Update(std::shared_ptr<whale::core::BaseFrame> frame,
                     vector<PersonObject> &cur_detected_person_objects,
                     vector<PersonObject> &tracked_objects,
                     vector<PersonObject> &tracked_iou_not_matched_objects,
                     vector<PersonObject> &expired_obejcts,
                     vector<PersonObject> &valid_leave_objects) {
    Update(frame->getcvMat(), cur_detected_person_objects, frame->camera_time,
           frame->frame_id, tracked_objects, tracked_iou_not_matched_objects,
           expired_obejcts, valid_leave_objects);

    // to be implement
}
void Tracker::Update(cv::Mat frame,
                     vector<PersonObject> &cur_detected_person_objects,
                     unsigned long long timestamp, unsigned long long frame_id,
                     vector<PersonObject> &tracked_objects,
                     vector<PersonObject> &tracked_iou_not_matched_objects,
                     vector<PersonObject> &expired_obejcts,
                     vector<PersonObject> &valid_leave_objects) {
    cur_process_frame_id_ = frame_id;
    cur_process_frame_timestamp_ = timestamp;
    p_cur_process_frame_mat_ = &frame;

    for (int i = 0; i < cur_detected_person_objects.size(); i++) {
        assert(!cur_detected_person_objects[i].is_current_frame_matched_or_new);
    }

    for (int i = 0; i < trackedPersonObjects.size(); i++) {
        trackedPersonObjects[i].is_current_frame_matched_or_new = false;
        trackedPersonObjects[i].is_current_body_detected = false;
        trackedPersonObjects[i].is_current_face_detected = false;
    }

    vector<MatchPair> match_pairs;

    // IOU match by face bbox
    //	IOUMatch(trackedPersonObjects, cur_detected_person_objects, face,
    //					 match_pairs);
    //	UpdateMatchedTrackedObjects(trackedPersonObjects,
    // cur_detected_person_objects,
    // match_pairs);

    // IOU match by body bbox
    IOUMatch(trackedPersonObjects, cur_detected_person_objects, body,
             match_pairs);
    UpdateMatchedTrackedObjects(trackedPersonObjects,
                                cur_detected_person_objects, match_pairs);

    for (int i = 0; i < trackedPersonObjects.size(); i++) {
        if (!trackedPersonObjects[i].is_current_frame_matched_or_new) {
            tracked_iou_not_matched_objects.push_back(trackedPersonObjects[i]);
        }
    }

    // feature match by body bbox
    FeatureMatch(trackedPersonObjects, cur_detected_person_objects, body,
                 match_pairs);
    UpdateMatchedTrackedObjects(trackedPersonObjects,
                                cur_detected_person_objects, match_pairs);

    UpdateUnMatchedTrackedObjectsStatus(timestamp, frame_id);

    // process expiredPersonObjects
    FeatureMatch(expiredPersonObjects, cur_detected_person_objects, body,
                 match_pairs);
    // update matched status
    UpdateMatchedExpiredObjects(expiredPersonObjects,
                                cur_detected_person_objects, match_pairs);
    // update unmatched expiredPersonObjects
    UpdateUnMatchedExpiredObjectsStatus(valid_leave_objects, timestamp,
                                        frame_id);

    AddUnMatchedCurDetectedObjects(trackedPersonObjects,
                                   cur_detected_person_objects);
    AccumulateDuration(trackedPersonObjects);

    // for (int i = 0; i < trackedPersonObjects.size(); i++) {
    // tracked_objects.push_back(trackedPersonObjects[i]);
    //}
    // todo fix接坣
    // tracked_objects = trackedPersonObjects;

    for (int i = 0; i < expiredPersonObjects.size(); i++) {
        expired_obejcts.push_back(expiredPersonObjects[i]);
    }
}

void Tracker::AccumulateDuration(vector<PersonObject> &tracked_person_objects) {
    for (int i = 0; i < tracked_person_objects.size(); i++) {
        if (tracked_person_objects[i].is_current_frame_matched_or_new &&
            tracked_person_objects[i].is_current_body_detected &&
            tracked_person_objects[i].last_updated_body_bbox.bbox_rect.area() /
                    (p_cur_process_frame_mat_->rows *
                     p_cur_process_frame_mat_->cols) >
                body_residence_area_thresh) {
            if (tracked_person_objects[i]
                    .is_previous_frame_body_bbox_area_bigger_than_thresh &&
                cur_process_frame_id_ -
                        tracked_person_objects[i]
                            .previous_body_bbox_area_bigger_than_thresh_frame_id <
                    body_bbox_area_bigger_than_thresh_frame_interval) {
                tracked_person_objects[i].duration +=
                    cur_process_frame_timestamp_ -
                    tracked_person_objects[i]
                        .previous_body_bbox_area_bigger_than_thresh_timestamp;
            }
            tracked_person_objects[i]
                .is_previous_frame_body_bbox_area_bigger_than_thresh = true;
            tracked_person_objects[i]
                .previous_body_bbox_area_bigger_than_thresh_timestamp =
                cur_process_frame_timestamp_;
            tracked_person_objects[i]
                .previous_body_bbox_area_bigger_than_thresh_frame_id =
                cur_process_frame_id_;
        }
    }
}

void Tracker::UpdateUnMatchedTrackedObjectsStatus(unsigned long long timestamp,
                                                  unsigned long long frame_id) {
    for (int i = 0; i < trackedPersonObjects.size(); i++) {
        if (trackedPersonObjects[i].is_current_frame_matched_or_new) continue;
        if (trackedPersonObjects[i].object_status == unconfirmed) {
            trackedPersonObjects.erase(trackedPersonObjects.begin() + i);
            i--;
            continue;
        } else if (frame_id - trackedPersonObjects[i]
                                  .last_updated_face_bbox.update_frame_id >
                       lost_track_thresh &&
                   frame_id - trackedPersonObjects[i]
                                  .last_updated_body_bbox.update_frame_id >
                       lost_track_thresh) {
            if (trackedPersonObjects[i].object_status == confirmed) {
                expiredPersonObjects.push_back(trackedPersonObjects[i]);
#ifdef DEBUG_MESSAGE
                printf("id: %d is add to expired\n",
                       trackedPersonObjects[i].id);
#endif
            }
#ifdef DEBUG_MESSAGE
            printf("id: %d is erased from trackedPersonObjects\n",
                   trackedPersonObjects[i].id);
#endif
            trackedPersonObjects.erase(trackedPersonObjects.begin() + i);
            i--;
            continue;
        }
    }
}

void Tracker::UpdateUnMatchedExpiredObjectsStatus(
    vector<PersonObject> &valid_leave_objects, unsigned long long timestamp,
    unsigned long long frame_id) {
    for (int i = 0; i < expiredPersonObjects.size(); i++) {
        if (frame_id - expiredPersonObjects[i]
                           .last_updated_face_bbox.update_frame_id >
                expired_to_delete_thresh &&
            frame_id - expiredPersonObjects[i]
                           .last_updated_body_bbox.update_frame_id >
                expired_to_delete_thresh) {
            // check valid passby
            if (CheckValidLeave(expiredPersonObjects[i])) {
#ifdef DEBUG_MESSAGE
                printf("+++++++++++++++valid leave, id: %d+++++++++++++\n",
                       expiredPersonObjects[i].id);
#endif
                valid_leave_objects.push_back(expiredPersonObjects[i]);
            }
            expiredPersonObjects.erase(expiredPersonObjects.begin() + i);
            i--;
        }
    }
}

void Tracker::GenValueTable(vector<vector<double>> &table,
                            vector<PersonObject> &tracked_person_objects,
                            vector<PersonObject> &cur_detect_person_objects,
                            BBoxType bbox_type) {
    assert(bbox_type == face or bbox_type == body);
    table = vector<vector<double>>(tracked_person_objects.size());
    for (int i = 0; i < tracked_person_objects.size(); i++) {
        table[i] = vector<double>(cur_detect_person_objects.size());
    }
    for (int i = 0; i < tracked_person_objects.size(); i++) {
        for (int j = 0; j < cur_detect_person_objects.size(); j++) {
            if (tracked_person_objects[i].is_current_frame_matched_or_new ||
                cur_detect_person_objects[j].is_current_frame_matched_or_new) {
                table[i][j] = MAX_DIST;
            } else {
                if (bbox_type == face) {
                    table[i][j] =
                        1 - GetIOU(tracked_person_objects[i]
                                       .last_updated_face_bbox.bbox_rect,
                                   cur_detect_person_objects[j]
                                       .last_updated_face_bbox.bbox_rect);
                } else if (bbox_type == body) {
                    table[i][j] =
                        1 - GetIOU(tracked_person_objects[i]
                                       .last_updated_body_bbox.bbox_rect,
                                   cur_detect_person_objects[j]
                                       .last_updated_body_bbox.bbox_rect);
                }
            }
        }
    }
}

void Tracker::FindBestMatch(vector<vector<double>> &table, int &min_i_index,
                            int &min_j_index, float &min_value) {
    for (int i = 0; i < table.size(); i++) {
        for (int j = 0; j < table[0].size(); j++) {
            if (table[i][j] < min_value) {
                min_value = table[i][j];
                min_i_index = i;
                min_j_index = j;
            }
        }
    }
}

void Tracker::IOUMatch(vector<PersonObject> &tracked_person_objects,
                       vector<PersonObject> &cur_detected_person_objects,
                       BBoxType bbox_type, vector<MatchPair> &match_pairs) {
    match_pairs.clear();
    vector<int> tracked_indexes;  // indexes of tracked_person_objects that need
                                  // to be matched
    for (int i = 0; i < tracked_person_objects.size(); i++) {
        if (!tracked_person_objects[i].is_current_frame_matched_or_new) {
            tracked_indexes.push_back(i);
        }
    }
    vector<int> detected_indexes;  // indexes of cur_detect_person_objects that
                                   // need to be matched
    for (int i = 0; i < cur_detected_person_objects.size(); i++) {
        if (!cur_detected_person_objects[i].is_current_frame_matched_or_new) {
            if (bbox_type == face &&
                cur_detected_person_objects[i].is_current_face_detected) {
                detected_indexes.push_back(i);
            } else if (bbox_type == body && cur_detected_person_objects[i]
                                                .is_current_body_detected) {
                detected_indexes.push_back(i);
            }
        }
    }

    if (tracked_indexes.empty() || detected_indexes.empty()) {
        return;
    }

    vector<vector<double>> table;
    table.resize(tracked_indexes.size(),
                 vector<double>(detected_indexes.size(), 1.0));
    for (int i = 0; i < tracked_indexes.size(); i++) {
        for (int j = 0; j < detected_indexes.size(); j++) {
            if (bbox_type == face) {
                double cost =
                    1.0 -
                    GetIOU(tracked_person_objects[tracked_indexes[i]]
                               .last_updated_face_bbox.bbox_rect,
                           cur_detected_person_objects[detected_indexes[j]]
                               .last_updated_face_bbox.bbox_rect);
                table[i][j] = cost;
            } else if (bbox_type == body) {
                double cost =
                    1.0 -
                    GetIOU(tracked_person_objects[tracked_indexes[i]]
                               .last_updated_body_bbox.bbox_rect,
                           cur_detected_person_objects[detected_indexes[j]]
                               .last_updated_body_bbox.bbox_rect);
                table[i][j] = cost;
            }
        }
    }
#ifdef DEBUG_MESSAGE
//            DEBUG_PRINT();
//            printf("IOU cost matrix: \n");
//            printf("%15s", "");
//            for (int j = 0; j < detected_indexes.size(); j++) {
//                printf("%15d",
//                cur_detected_person_objects[detected_indexes[j]].id);
//            }
//            printf("\n");
//            for (int i = 0; i < tracked_indexes.size(); i++) {
//                printf("%15d", tracked_person_objects[tracked_indexes[i]].id);
//                for (int j = 0; j < detected_indexes.size(); j++) {
//                    printf("%15f", table[i][j]);
//                }
//                printf("\n");
//            }
#endif

    HungarianAlgorithm HungAlgo;
    vector<int> assignment;
    HungAlgo.Solve(table, assignment);
    for (int i = 0; i < assignment.size(); i++) {
        if (assignment[i] == -1) continue;
        if (table[i][assignment[i]] < 1.0 - iou_thresh) {
            match_pairs.push_back(
                MatchPair{.i_index = tracked_indexes[i],
                          .j_index = detected_indexes[assignment[i]]});
            //                    UpdateMatchedObjectStatus(tracked_person_objects[tracked_indexes[i]],
            //                            cur_detected_person_objects[detected_indexes[assignment[i]]]);
        }
    }
}

void Tracker::FeatureMatch(vector<PersonObject> &expired_person_objects,
                           vector<PersonObject> &cur_detected_person_objects,
                           BBoxType bbox_type, vector<MatchPair> &match_pairs) {
    match_pairs.clear();
    vector<int> expired_indexes;  // indexes of expired_person_objects that need
                                  // to be matched
    for (int i = 0; i < expired_person_objects.size(); i++) {
        if (!expired_person_objects[i].is_current_frame_matched_or_new &&
            expired_person_objects[i].object_status == confirmed) {
            if (bbox_type == body &&
                !expired_person_objects[i].feature_messages.empty()) {
                expired_indexes.push_back(i);
            }
        }
    }
    vector<int> detected_indexes;  // indexes of cur_detect_person_objects that
                                   // need to be matched
    for (int i = 0; i < cur_detected_person_objects.size(); i++) {
        if (!cur_detected_person_objects[i].is_current_frame_matched_or_new) {
            if (bbox_type == face &&
                cur_detected_person_objects[i].is_current_face_detected) {
                detected_indexes.push_back(i);
            } else if (bbox_type == body &&
                       cur_detected_person_objects[i]
                           .is_current_body_detected &&
                       cur_detected_person_objects[i]
                                   .last_updated_body_bbox.bbox_rect.area() /
                               (p_cur_process_frame_mat_->rows *
                                p_cur_process_frame_mat_->cols) >
                           bbox_area_extract_feature_thresh) {
                detected_indexes.push_back(i);
            }
        }
    }
    if (expired_indexes.empty() || detected_indexes.empty()) {
        return;
    }
    vector<vector<double>> table;
    table.resize(expired_indexes.size(),
                 vector<double>(detected_indexes.size(), 1.0));
    for (int i = 0; i < expired_indexes.size(); i++) {
        for (int j = 0; j < detected_indexes.size(); j++) {
            if (bbox_type == face) {
            } else if (bbox_type == body) {
                if (cur_detected_person_objects[detected_indexes[j]]
                        .last_updated_body_feature_message.tengine_feature
                        .empty()) {
                    // tengine_feature_identifier_.GetFeature(
                    // 		*p_cur_process_frame_mat_,
                    // 		cur_detected_person_objects[detected_indexes[j]]
                    // 				.last_updated_body_bbox.bbox_rect,
                    // 		cur_detected_person_objects[detected_indexes[j]]
                    // 				.last_updated_body_feature_message.tengine_feature);
                    // for reid tengine
                    cv::Mat image = *p_cur_process_frame_mat_;

                    int64_t cur_time = SysPublicTool::getCurrentTime();
                    whale::core::sdk::reid(
                        image(cur_detected_person_objects[detected_indexes[j]]
                                  .last_updated_body_bbox.bbox_rect),
                        cur_detected_person_objects[detected_indexes[j]]
                            .last_updated_body_feature_message.tengine_feature);
                    int64_t duration =
                        SysPublicTool::getCurrentTime() - cur_time;

                    ServiceMetricManager::add(MetricAction::kAlgoModelMonitor,
                                              "cv_reid_count", 1);
                    ServiceMetricManager::add(MetricAction::kAlgoModelMonitor,
                                              "cv_reid_cost", duration);

                    cur_detected_person_objects[detected_indexes[j]]
                        .last_updated_body_feature_message.update_frame_id =
                        cur_process_frame_id_;
                    cur_detected_person_objects[detected_indexes[j]]
                        .last_updated_body_feature_message.update_timestamp =
                        cur_process_frame_timestamp_;
                }
                double min_cost = 1;
                for (int k = 0; k < expired_person_objects[expired_indexes[i]]
                                        .feature_messages.size();
                     k++) {
                    double cost =
                        1 - CalcCosSimilarity(
                                expired_person_objects[expired_indexes[i]]
                                    .feature_messages[k]
                                    .tengine_feature,
                                cur_detected_person_objects[detected_indexes[j]]
                                    .last_updated_body_feature_message
                                    .tengine_feature);
                    if (cost < min_cost) {
                        min_cost = cost;
                    }
                }
                table[i][j] = min_cost;
            }
        }
    }
#ifdef DEBUG_MESSAGE
    DEBUG_PRINT();
    printf("match rate cost matrix: \n");
    printf("%15s", "");
    for (int j = 0; j < detected_indexes.size(); j++) {
        printf("%15d", cur_detected_person_objects[detected_indexes[j]].id);
    }
    printf("\n");
    for (int i = 0; i < expired_indexes.size(); i++) {
        printf("%15d", expired_person_objects[expired_indexes[i]].id);
        for (int j = 0; j < detected_indexes.size(); j++) {
            printf("%15f", table[i][j]);
        }
        printf("\n");
    }
#endif
    HungarianAlgorithm HungAlgo;
    vector<int> assignment;
    HungAlgo.Solve(table, assignment);
    for (int i = 0; i < assignment.size(); i++) {
        if (assignment[i] == -1) continue;
        if (table[i][assignment[i]] < 1.0 - cos_similarity_thresh) {
            match_pairs.push_back(
                MatchPair{.i_index = expired_indexes[i],
                          .j_index = detected_indexes[assignment[i]]});
        }
    }
}

void Tracker::UpdateMatchedTrackedObjects(
    vector<PersonObject> &tracked_person_objects,
    vector<PersonObject> &cur_detected_person_objects,
    vector<MatchPair> &match_pairs) {
    for (int i = 0; i < match_pairs.size(); i++) {
        UpdateMatchedObjectStatus(
            tracked_person_objects[match_pairs[i].i_index],
            cur_detected_person_objects[match_pairs[i].j_index]);
    }
}

void Tracker::UpdateMatchedExpiredObjects(
    vector<PersonObject> &expired_person_objects,
    vector<PersonObject> &cur_detected_person_objects,
    vector<MatchPair> &match_pairs) {
    for (int i = 0; i < match_pairs.size(); i++) {
        UpdateMatchedObjectStatus(
            expired_person_objects[match_pairs[i].i_index],
            cur_detected_person_objects[match_pairs[i].j_index]);
    }
    for (int i = 0; i < expired_person_objects.size(); i++) {
        if (expired_person_objects[i].is_current_frame_matched_or_new) {
            trackedPersonObjects.push_back(expired_person_objects[i]);
#ifdef DEBUG_MESSAGE
            printf("id: %d is move from expired into tracked\n",
                   expired_person_objects[i].id);
#endif
            expired_person_objects.erase(expired_person_objects.begin() + i);
            i--;
        }
    }
}

void Tracker::UpdateMatchedObjectStatus(
    PersonObject &tracked_person_object,
    PersonObject &cur_detect_person_object) {
    tracked_person_object.is_current_frame_matched_or_new = true;
    cur_detect_person_object.is_current_frame_matched_or_new = true;
    if (cur_detect_person_object.is_current_body_detected) {
        // todo: if detected object already has feature, update
        // tracked_person_object's feature check if tracked_person_object need
        // extract body feature extract feature
        if (IsObjectNeedExtractFeature(tracked_person_object) &&
            cur_detect_person_object.last_updated_body_bbox.bbox_rect.area() /
                    (p_cur_process_frame_mat_->rows *
                     p_cur_process_frame_mat_->cols) >
                bbox_area_extract_feature_thresh) {
            if (cur_detect_person_object.last_updated_body_feature_message
                    .tengine_feature.empty()) {
                // tengine_feature_identifier_.GetFeature(
                // 		*p_cur_process_frame_mat_,
                // 		cur_detect_person_object.last_updated_body_bbox.bbox_rect,
                // 		cur_detect_person_object.last_updated_body_feature_message
                // 				.tengine_feature);

                // for reid
                cv::Mat image = *p_cur_process_frame_mat_;
                int64_t cur_time = SysPublicTool::getCurrentTime();
                whale::core::sdk::reid(
                    image(cur_detect_person_object.last_updated_body_bbox
                              .bbox_rect),
                    cur_detect_person_object.last_updated_body_feature_message
                        .tengine_feature);
                int64_t duration = SysPublicTool::getCurrentTime() - cur_time;

                ServiceMetricManager::add(MetricAction::kAlgoModelMonitor,
                                          "cv_reid_count", 1);
                ServiceMetricManager::add(MetricAction::kAlgoModelMonitor,
                                          "cv_reid_cost", duration);

                cur_detect_person_object.last_updated_body_feature_message
                    .update_frame_id = cur_process_frame_id_;
                cur_detect_person_object.last_updated_body_feature_message
                    .update_timestamp = cur_process_frame_timestamp_;
            }
            tracked_person_object.last_updated_body_feature_message =
                cur_detect_person_object.last_updated_body_feature_message;
            tracked_person_object.feature_messages.push_back(
                cur_detect_person_object.last_updated_body_feature_message);
            while (tracked_person_object.feature_messages.size() >
                   feature_vec_size) {
                tracked_person_object.feature_messages.erase(
                    tracked_person_object.feature_messages.begin());
            }
        }

        tracked_person_object.body_hit_num += 1;
        if (tracked_person_object.body_hit_num > hit_num_thresh) {
            tracked_person_object.object_status = confirmed;
        }

        tracked_person_object.is_current_body_detected = true;
        tracked_person_object.last_updated_body_bbox =
            cur_detect_person_object.last_updated_body_bbox;
        tracked_person_object.body_trajectory.push_back(
            cur_detect_person_object.last_updated_body_bbox);
        while (tracked_person_object.body_trajectory.size() > trajectory_size) {
            tracked_person_object.body_trajectory.erase(
                tracked_person_object.body_trajectory.begin());
        }
    } else {
        tracked_person_object.is_current_body_detected = false;
    }
    if (cur_detect_person_object.is_current_face_detected) {
        tracked_person_object.is_current_face_detected = true;
        tracked_person_object.last_updated_face_bbox =
            cur_detect_person_object.last_updated_face_bbox;
        tracked_person_object.face_trajectory.push_back(
            cur_detect_person_object.last_updated_face_bbox);
        while (tracked_person_object.face_trajectory.size() > trajectory_size) {
            tracked_person_object.face_trajectory.erase(
                tracked_person_object.face_trajectory.begin());
        }
    } else {
        tracked_person_object.is_current_face_detected = false;
    }
}

bool Tracker::IsObjectNeedExtractFeature(PersonObject person_object) {
    if (person_object.object_status == unconfirmed ||
        person_object.last_updated_body_feature_message.tengine_feature
            .empty()) {
        return true;
    }
    if (cur_process_frame_id_ -
            person_object.last_updated_body_feature_message.update_frame_id >
        feature_extract_thresh) {
        return true;
    }
    return false;
}

void Tracker::UpdateMatchedPersonObject(
    vector<vector<double>> &table, vector<PersonObject> &tracked_person_objects,
    vector<PersonObject> &cur_detect_person_objects, int i_index, int j_index) {
    if (tracked_person_objects[i_index].id == 8) {
        printf("test\n");
    }

    for (int j = 0; j < cur_detect_person_objects.size(); j++) {
        table[i_index][j] = MAX_DIST;
    }
    for (int i = 0; i < tracked_person_objects.size(); i++) {
        table[i][j_index] = MAX_DIST;
    }

    PersonObject *tracked_object = &(tracked_person_objects[i_index]);
    PersonObject *cur_det_object = &(cur_detect_person_objects[j_index]);

    tracked_object->is_current_frame_matched_or_new = true;
    cur_det_object->is_current_frame_matched_or_new = true;
    if (cur_detect_person_objects[j_index].is_current_body_detected) {
        tracked_object->is_current_body_detected = true;
        tracked_object->last_updated_body_bbox =
            cur_det_object->last_updated_body_bbox;
        tracked_object->body_trajectory.push_back(
            cur_det_object->last_updated_body_bbox);
        while (tracked_object->body_trajectory.size() > trajectory_size) {
            tracked_object->body_trajectory.erase(
                tracked_object->body_trajectory.begin());
        }
    } else {
        tracked_object->is_current_body_detected = false;
    }
    if (cur_detect_person_objects[j_index].is_current_face_detected) {
        tracked_object->is_current_face_detected = true;
        tracked_object->last_updated_face_bbox =
            cur_det_object->last_updated_face_bbox;
        tracked_object->face_trajectory.push_back(
            cur_det_object->last_updated_face_bbox);
        while (tracked_object->face_trajectory.size() > trajectory_size) {
            tracked_object->face_trajectory.erase(
                tracked_object->face_trajectory.begin());
        }
    } else {
        tracked_object->is_current_face_detected = false;
    }
}

void Tracker::UpdateUnMatchedTrackedAndExpiredObjectsStatus(
    vector<PersonObject> &tracked_person_objects,
    vector<PersonObject> &expired_person_objects, unsigned long long timestamp,
    unsigned long long frame_id) {
    for (int i = 0; i < tracked_person_objects.size(); i++) {
        if (tracked_person_objects[i].is_current_frame_matched_or_new) continue;
        if (frame_id - tracked_person_objects[i]
                           .last_updated_face_bbox.update_frame_id >
                lost_track_thresh &&
            frame_id - tracked_person_objects[i]
                           .last_updated_body_bbox.update_frame_id >
                lost_track_thresh) {
            expired_person_objects.push_back(tracked_person_objects[i]);
            tracked_person_objects.erase(tracked_person_objects.begin() + i);
            i--;
        }
    }

    for (int i = 0; i < expired_person_objects.size(); i++) {
        if (frame_id - expired_person_objects[i]
                           .last_updated_face_bbox.update_frame_id >
                expired_to_delete_thresh &&
            frame_id - expired_person_objects[i]
                           .last_updated_face_bbox.update_frame_id >
                expired_to_delete_thresh) {
            expired_person_objects.erase(expired_person_objects.begin() + i);
            i--;
        }
    }
}

bool Tracker::CheckValidLeave(PersonObject &person_object) {
    if (valid_leave_with_reid) {
        return !(person_object.body_trajectory.size() < 15 ||
                 person_object.feature_messages.empty());
    } else {
        return person_object.body_trajectory.size() >= 15;
    }
}

//        bool Tracker::CheckValidTrajectory(PersonObject &person_object) {
//            if (GetIOU(person_object.body_trajectory[0].bbox_rect,
//            person_object.body_trajectory[person_object.body_trajectory.size()-1].bbox_rect)
//            > 0.8) {
//                if (IsBoxNearBorder(person_object.last_updated_body_bbox,
//                fra))
//            }
//        }

bool Tracker::IsBoxNearBorder(Rect_<float> bbox, int width, int height,
                              int thresh) {
    assert(thresh > 0 && thresh < 50);
    if (bbox.x < thresh || bbox.x + bbox.width > width - thresh ||
        bbox.y + bbox.height > height - thresh) {
        return true;
    }
    return false;
}

bool Tracker::CheckLeave(PersonObject &person_object) {
    if (!person_object.is_current_frame_matched_or_new) {
        if (cur_process_frame_id_ -
                    person_object.last_updated_face_bbox.update_frame_id >
                lost_track_thresh &&
            cur_process_frame_id_ -
                    person_object.last_updated_body_bbox.update_frame_id >
                lost_track_thresh) {
            return true;
        }
    }
    return false;
}

void Tracker::AddUnMatchedCurDetectedObjects(
    vector<PersonObject> &tracked_person_objects,
    vector<PersonObject> &cur_detect_person_objects) {
    for (int i = 0; i < cur_detect_person_objects.size(); i++) {
        if (!cur_detect_person_objects[i].is_current_frame_matched_or_new) {
            cur_detect_person_objects[i].is_current_frame_matched_or_new = true;
            cur_detect_person_objects[i].id = global_id;
            global_id++;
            tracked_person_objects.push_back(cur_detect_person_objects[i]);
        }
    }
}

double Tracker::GetIOU(Rect_<float> bb_test, Rect_<float> bb_gt) {
    float in = (bb_test & bb_gt).area();
    float un = bb_test.area() + bb_gt.area() - in;
    if (un < DBL_EPSILON) return 0;
    return (double)(in / un);
}

double Tracker::CalcCosSimilarity(float *p_feature_a, float *p_feature_b,
                                  int size) {
    double matmul = 0;
    double norm_a = 0;
    double norm_b = 0;
    for (int i = 0; i < size; i++) {
        matmul += *(p_feature_a + i) * (*(p_feature_b + i));
        norm_a += *(p_feature_a + i) * (*(p_feature_a + i));
        norm_b += *(p_feature_b + i) * (*(p_feature_b + i));
    }
    double ret = matmul / (sqrt(norm_a) * sqrt(norm_b));
    //        printf("cosine_similarity:%f\n", ret);
    //        if ( ret < -100) {
    //            for (int i = 0; i < 20; i++) {
    //                printf("%f, %f\n", tengine_featrues[i],
    //                other.tengine_featrues[i]);
    //            }
    //        }
    return ret;
}

double Tracker::CalcCosSimilarity(vector<float> feature_a,
                                  vector<float> feature_b) {
    assert(feature_a.size() == feature_b.size());
    double matmul = 0;
    double norm_a = 0;
    double norm_b = 0;
    for (int i = 0; i < feature_a.size(); i++) {
        matmul += feature_a[i] * feature_b[i];
        norm_a += feature_a[i] * feature_a[i];
        norm_b += feature_b[i] * feature_b[i];
    }
    double ret = matmul / (sqrt(norm_a) * sqrt(norm_b));
    return ret;
}

double Tracker::CalcMatchRate(Mat &descriptor_a, Mat &descriptor_b) {
    auto matcher = DescriptorMatcher::create(cv::DescriptorMatcher::BRUTEFORCE);
    std::vector<DMatch> matches;
    matcher->match(descriptor_a, descriptor_b, matches);
    int matched = 0;
    for (int i = 0; i < matches.size(); i++) {
        auto &match = matches[i];
        if (abs(match.distance) < 100) {
            matched++;
        }
        printf("%f\n", match.distance);
    }
    return (double)matched / (double)matches.size();
}

cv::Mat Tracker::GetDescriptor(cv::Mat &img, cv::Rect_<float> &det) {
    Mat img_rec = img(det);
    Mat transMat;
    cv::cvtColor(img_rec, transMat, cv::COLOR_BGR2GRAY);
    //            cv::Mat transMat = img(det).clone();
    //            cv::cvtColor(img, transMat, cv::COLOR_RGB2GRAY);

    // auto detector = cv::GFTTDetector::create();
    auto detector = cv::ORB::create(10000);
    Ptr<DescriptorExtractor> descriptorExtractor = detector;
    std::vector<cv::KeyPoint> keypoint;
    cv::Mat descriptor;
    // detector->detectAndCompute(transMat, cv::noArray(), keypoint,
    // descriptor);
    detector->detect(transMat, keypoint);
    descriptorExtractor->compute(transMat, keypoint, descriptor);
    return descriptor;
}

bool Tracker::onboundary(const cv::Rect &box, float width, float height) {
    /*
    if (box.x <= width * 0.05) {
                    return true;
    }
    if (box.x + box.width >= width * 0.95) {
                    return true;
    }
    if (box.y <= height * 0.05) {
                    return true;
    }
    if (box.y + box.height >= height * 0.95) {
                    return true;
    }
    return false;
    */
    return true;
}

}  // namespace vision
}  // namespace whale
