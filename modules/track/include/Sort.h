
#pragma once

#include <unistd.h>
#include <fstream>
#include <iomanip>	// to format image names using setw() and setfill()
#include <iostream>
#include <set>
#include <map>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/tracking.hpp>

#include "Hungarian.h"
#include "KalmanTracker.h"
#include "AlgoData.h"

using namespace whale::core;

namespace whale {
namespace vision {

struct SORT {
// global variables for counting
#define CNUM 20
	int total_frames = 0;
	double total_time = 0.0;

	cv::Scalar_<int> randColor[CNUM];

	int frame_count = 0;
	int max_age = 10;
	int min_hits = 5;
	double iouThreshold = 0.3;
	std::vector<KalmanTracker> trackers;

	// variables used in the for-loop
	std::vector<cv::Rect_<float>> predictedBoxes;
	std::vector<std::vector<double>> iouMatrix;
	std::vector<int> assignment;
	std::set<int> unmatchedDetections;
	std::set<int> unmatchedTrajectories;
	std::set<int> allItems;
	std::set<int> matchedItems;
	std::vector<cv::Point> matchedPairs;
	std::vector<Box> frameTrackingResult;
	unsigned int trkNum = 0;
	unsigned int detNum = 0;

	double cycle_time = 0.0;
	int64 start_time = 0;

	SORT(int max_age) {
		this->max_age = max_age;
		// tracking id relies on this, so we have to reset it in each seq.
		KalmanTracker::kf_count = 0;
		cv::RNG rng(0xFFFFFFFF);
		for (int i = 0; i < CNUM; i++) {
			rng.fill(randColor[i], cv::RNG::UNIFORM, 0, 256);
		}
	}

	// Computes IOU between two bounding boxes
	double GetIOU(cv::Rect_<float> bb_test, cv::Rect_<float> bb_gt) {
		float in = (bb_test & bb_gt).area();
		float un = bb_test.area() + bb_gt.area() - in;

		if (un < DBL_EPSILON) {
			return 0;
		}

		return (double)(in / un);
	}

	std::vector<Box> update(const std::vector<Box> &detFrameData) {
		total_frames++;
		frame_count++;
		start_time = cv::getTickCount();

		std::map<int, int> detect_id_in_tracker;	// <tracker, detect>

		// the first frame meet
		if (trackers.size() == 0) {
			// initialize kalman trackers using first detections.
			for (unsigned int i = 0; i < detFrameData.size(); i++) {
				KalmanTracker trk = KalmanTracker(detFrameData[i].rect);
				trackers.push_back(trk);
			}
			return std::vector<Box>();
		}

		// 3.1. get predicted locations from existing trackers.
		predictedBoxes.clear();

		for (auto it = trackers.begin(); it != trackers.end();) {
			cv::Rect_<float> pBox = (*it).predict();
			if (pBox.x >= 0 && pBox.y >= 0) {
				predictedBoxes.push_back(pBox);
				it++;
			} else {
				it = trackers.erase(it);
				// LOG(INFO) << "tracker predict out of bound--tracker deleted" <<
				// std::endl;
			}
		}
		///////////////////////////////////////
		// 3.2. associate detections to tracked object (both represented as bounding
		// boxes)
		// dets : detFrameData[fi]
		trkNum = predictedBoxes.size();
		detNum = detFrameData.size();

		iouMatrix.clear();
		iouMatrix.resize(trkNum, std::vector<double>(detNum, 0));

		for (unsigned int i = 0; i < trkNum;
				 i++)	 // compute iou matrix as a distance matrix
		{
			for (unsigned int j = 0; j < detNum; j++) {
				// use 1-iou because the hungarian algorithm computes a minimum-cost
				// assignment.
				iouMatrix[i][j] = 1 - GetIOU(predictedBoxes[i], detFrameData[j].rect);
			}
		}

		// solve the assignment problem using hungarian algorithm.
		// the resulting assignment is [track(prediction) : detection], with
		// len=preNum
		HungarianAlgorithm HungAlgo;
		assignment.clear();
		HungAlgo.Solve(iouMatrix, assignment);

		// find matches, unmatched_detections and unmatched_predictions
		unmatchedTrajectories.clear();
		unmatchedDetections.clear();
		allItems.clear();
		matchedItems.clear();

		if (detNum > trkNum)	//	there are unmatched detections
		{
			for (unsigned int n = 0; n < detNum; n++) {
				allItems.insert(n);
			}

			for (unsigned int i = 0; i < trkNum; ++i) {
				matchedItems.insert(assignment[i]);
			}

			std::set_difference(
					allItems.begin(), allItems.end(), matchedItems.begin(),
					matchedItems.end(),
					std::insert_iterator<std::set<int>>(unmatchedDetections,
																							unmatchedDetections.begin()));
		} else if (detNum < trkNum)	 // there are unmatched trajectory/predictions
		{
			for (unsigned int i = 0; i < trkNum; ++i) {
				// unassigned label will be set as -1 in the assignment algo
				if (assignment[i] == -1) {
					unmatchedTrajectories.insert(i);
				}
			}
		}

		// filter out matched with low IOU
		matchedPairs.clear();
		for (unsigned int i = 0; i < trkNum; ++i) {
			if (assignment[i] == -1) {
				continue;
			}
			if (1 - iouMatrix[i][assignment[i]] < iouThreshold) {
				unmatchedTrajectories.insert(i);
				unmatchedDetections.insert(assignment[i]);
			} else {
				matchedPairs.push_back(cv::Point(i, assignment[i]));
			}
		}

		///////////////////////////////////////
		// 3.3. updating trackers

		// update matched trackers with assigned detections.
		// each prediction is corresponding to a tracker
		int detIdx, trkIdx;
		for (unsigned int i = 0; i < matchedPairs.size(); i++) {
			trkIdx = matchedPairs[i].x;
			detIdx = matchedPairs[i].y;
			trackers[trkIdx].update(detFrameData[detIdx].rect);
			detect_id_in_tracker[trackers[trkIdx].kf_count] = detIdx;
		}

		// create and initialise new trackers for unmatched detections
		for (auto umd : unmatchedDetections) {
			// LOG(INFO) << "unmatched detection-----create new tracker" ;
			KalmanTracker tracker = KalmanTracker(detFrameData[umd].rect);
			trackers.push_back(tracker);
			detect_id_in_tracker[tracker.kf_count] = detIdx;
		}

		// get trackers' output
		frameTrackingResult.clear();
		for (auto it = trackers.begin(); it != trackers.end();) {
			if (((*it).m_time_since_update < 1) &&
					((*it).m_hit_streak >= min_hits || frame_count <= min_hits)) {
				Box res;
				res.rect = (*it).lastRect;
				res.id = (*it).m_id + 1;
				// res.frame = frame_count;
				frameTrackingResult.push_back(res);
				it++;
			} else {
				it++;
			}

			// remove dead tracklet
			if (it != trackers.end() && (*it).m_time_since_update > max_age)
				it = trackers.erase(it);
		}

		cycle_time = (double)(cv::getTickCount() - start_time);
		total_time += cycle_time / cv::getTickFrequency();

		return frameTrackingResult;
	}
};
}	 // namespace vision
}	 // end namespace whale
