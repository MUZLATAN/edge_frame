//
// Created by neptune on 19-9-2.
//

#ifndef FMS_TSTTRACKER_HPP
#define FMS_TSTTRACKER_HPP

#include <vector>
#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include "AlgoData.h"

using namespace std;
using namespace cv;
namespace whale {
namespace vision {

enum BBoxType { face, body };
enum ObjectStatus { unconfirmed, confirmed };
enum ZoneType { unclearZone, enterZone,exitZone};
enum CustmType {ped_t_passby,ped_t_in,ped_t_clerk};


struct MatchPair {
	int i_index;
	int j_index;
};

struct BBoxMessage {
	unsigned long long update_timestamp;
	unsigned long long update_frame_id;
	float score;
	cv::Rect_<float> bbox_rect;
};

struct FeatureMessage {
	unsigned long long update_timestamp;
	unsigned long long update_frame_id;
	vector<float> tengine_feature;
};

class PersonObject {
 public:
	std::map<int, int> car_state;
	std::map<int, float> iou_state;
	int id = -1;
	float face_score = 0.0f;
	string face_id = "";
	bool is_current_frame_matched_or_new = false;
	bool is_previous_frame_body_bbox_area_bigger_than_thresh = false;
	unsigned long long previous_body_bbox_area_bigger_than_thresh_timestamp;
	unsigned long long previous_body_bbox_area_bigger_than_thresh_frame_id;
	unsigned long long duration = 0;
	bool occured_face_approach_event = false;
	bool occured_face_front_event = false;
	bool is_current_face_detected = false;
	bool is_current_face_front = false;
	bool occured_body_approach_event = false;
	bool occured_body_leave_event = false;
	bool is_current_body_detected = false;
	unsigned long long first_occured_body_approach_timestamp = 0;
	ObjectStatus object_status = unconfirmed;
	//for capture feature
	ZoneType first_zone_type = unclearZone;
	ZoneType cur_zone_type = unclearZone;
	double cur_score = 0;

	int body_hit_num = 0;
	int body_miss_num = 0;
	unsigned long long first_occured_face_approach_timestamp = 0;
	unsigned long long first_occured_face_front_timestamp = 0,
										 last_occured_face_front_timestamp = 0;

	BBoxMessage first_updated_face_bbox, first_updated_body_bbox,
			last_updated_face_bbox, last_updated_body_bbox;
	FeatureMessage last_updated_body_feature_message;
	vector<BBoxMessage> face_trajectory;
	vector<BBoxMessage> body_trajectory;
	vector<FeatureMessage> feature_messages;

	//for hsb
	CustmType ped_type = ped_t_passby;
	int64_t in_hsb_t = 0;
	int64_t out_hsb_t = 0;

 public:
	PersonObject(BBoxMessage *face_bbox, BBoxMessage *body_bbox,
							 unsigned long long timestamp, unsigned long long frame_id,
							 int p_id) {
		id = p_id;
		if (face_bbox != nullptr) {
			face_trajectory.push_back(*face_bbox);
			is_current_face_detected = true;
			last_updated_face_bbox = *face_bbox;
			first_updated_face_bbox = *face_bbox;
			face_trajectory.push_back(*face_bbox);
		}
		if (body_bbox != nullptr) {
			body_trajectory.push_back(*body_bbox);
			is_current_body_detected = true;
			last_updated_body_bbox = *body_bbox;
			first_updated_body_bbox = *body_bbox;
			body_trajectory.push_back(*body_bbox);
		}
		is_current_frame_matched_or_new = false;
		for(int i = 0;i < 10;i++) {
			car_state[i] = -1;
		}
	}
};
class Tracker {
 private:
	unsigned long long cur_process_frame_id_;
	unsigned long long cur_process_frame_timestamp_;
	Mat *p_cur_process_frame_mat_;
	vector<PersonObject> trackedPersonObjects;
	vector<PersonObject> expiredPersonObjects;

 public:
	unsigned long long global_id = 0;
	// threshold
	double iou_thresh = 0.45;
	double cos_similarity_thresh = 0.5;
	int hit_num_thresh = 3;
	int trajectory_size = 100;
	int feature_vec_size = 10;
	int body_bbox_area_bigger_than_thresh_frame_interval = 10;

	// frame nums of lost track, larger than it need to be moved to
	// expiredPersonObjects
	int lost_track_thresh = 10;

	// frame nums of lost track, larger than it need to be removed
	int expired_to_delete_thresh = 100;

	// if cur_frame_id - last_feature_extract_frame_id > feature_extract_thresh,
	// need extract feature
	int feature_extract_thresh = 2;

	//(bbox area/frame area) larger than it can be extract feature
	double bbox_area_extract_feature_thresh = 50000.0 / (960.0 * 540.0);
	//    double bbox_area_extract_feature_thresh = 0;

	// if body bbox area > body_residence_area_thresh, add to residence time
	double body_residence_area_thresh = 0.7 * bbox_area_extract_feature_thresh;

	vector<PersonObject> &GetTrackedPersonObjects() {
		return trackedPersonObjects;
	};
	unsigned long long GetCurrentProcessFrameTimestamp() {
		return cur_process_frame_timestamp_;
	};
	unsigned long long GetCurrentProcessFrameID() {
		return cur_process_frame_id_;
	};
	vector<PersonObject> &GetExpiredPersonObjects() {
		return expiredPersonObjects;
	};
	bool valid_leave_with_reid= true;

 private:
	bool onboundary(const cv::Rect &box, float width, float height);
	cv::Mat GetDescriptor(cv::Mat &img, cv::Rect_<float> &det);
	double GetIOU(Rect_<float> bb_test, Rect_<float> bb_gt);
	double CalcCosSimilarity(float *p_feature_a, float *p_feature_b, int size);
	double CalcCosSimilarity(vector<float> feature_a, vector<float> feature_b);
	double CalcMatchRate(Mat &descriptor_a, Mat &descriptor_b);
	bool CheckValidLeave(PersonObject &person_object);
	bool CheckValidTrajectory(PersonObject &person_object);
	bool IsBoxNearBorder(Rect_<float> bbox, int width, int height, int thresh);
	bool CheckLeave(PersonObject &person_object);
	void GenValueTable(vector<vector<double>> &table,
										 vector<PersonObject> &tracked_person_objects,
										 vector<PersonObject> &cur_detect_person_objects,
										 BBoxType bbox_type);
	void IOUMatch(vector<PersonObject> &tracked_person_objects,
								vector<PersonObject> &cur_detected_person_objects,
								BBoxType bbox_type, vector<MatchPair> &match_pairs);
	void FindBestMatch(vector<vector<double>> &table, int &min_i_index,
										 int &min_j_index, float &min_value);
	void UpdateMatchedPersonObject(
			vector<vector<double>> &table,
			vector<PersonObject> &tracked_person_objects,
			vector<PersonObject> &cur_detect_person_objects, int i_index,
			int j_index);
	bool IsObjectNeedExtractFeature(PersonObject person_object);
	void UpdateUnMatchedTrackedAndExpiredObjectsStatus(
			vector<PersonObject> &tracked_person_objects,
			vector<PersonObject> &expired_person_objects,
			unsigned long long timestamp, unsigned long long frame_id);
	void AddUnMatchedCurDetectedObjects(
			vector<PersonObject> &tracked_person_objects,
			vector<PersonObject> &cur_detect_person_objects);
	void UpdateUnMatchedTrackedObjectsStatus(unsigned long long timestamp,
																					 unsigned long long frame_id);
	void UpdateUnMatchedExpiredObjectsStatus(
			vector<PersonObject> &valid_leave_objects, unsigned long long timestamp,
			unsigned long long frame_id);
	void UpdateMatchedObjectStatus(PersonObject &tracked_person_object,
																 PersonObject &cur_detect_person_object);
	void UpdateMatchedTrackedObjects(
			vector<PersonObject> &tracked_person_objects,
			vector<PersonObject> &cur_detected_person_objects,
			vector<MatchPair> &match_pairs);
	void UpdateMatchedExpiredObjects(
			vector<PersonObject> &expired_person_objects,
			vector<PersonObject> &cur_detected_person_objects,
			vector<MatchPair> &match_pairs);
	void FeatureMatch(vector<PersonObject> &expired_person_objects,
										vector<PersonObject> &cur_detected_person_objects,
										BBoxType bbox_type, vector<MatchPair> &match_pairs);

	void AccumulateDuration(vector<PersonObject> &tracked_person_objects);

 public:
	Tracker(){};
	void Update(cv::Mat frame, vector<PersonObject> &cur_detected_person_objects,
							unsigned long long timestamp, unsigned long long frame_id,
							vector<PersonObject> &tracked_objects,
							vector<PersonObject> &tracked_iou_not_matched_objects,
							vector<PersonObject> &expired_obejcts,
							vector<PersonObject> &valid_leave_objects);
	void Update(std::shared_ptr<whale::core::BaseFrame> frame,
										 vector<PersonObject> &cur_detected_person_objects,
										 vector<PersonObject> &tracked_objects,
										 vector<PersonObject> &tracked_iou_not_matched_objects,
										 vector<PersonObject> &expired_obejcts,
										 vector<PersonObject> &valid_leave_objects);
};

}	 // namespace vision
}	 // namespace whale

#endif	// FMS_TRACKER_HPP
