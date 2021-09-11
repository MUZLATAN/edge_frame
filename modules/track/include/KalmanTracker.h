///////////////////////////////////////////////////////////////////////////////
// KalmanTracker.h: KalmanTracker Class Declaration

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/tracking.hpp>

namespace whale {
namespace vision {

// This class represents the internel state of individual tracked objects
// observed as bounding box.
class KalmanTracker {
 public:
	typedef cv::Rect_<float> KStateType;
	KalmanTracker() {
		init_kf(KStateType());
		m_time_since_update = 0;
		m_hits = 0;
		m_hit_streak = 0;
		m_age = 0;
		m_id = kf_count;
		// kf_count++;
	}
	KalmanTracker(KStateType initRect) {
		init_kf(initRect);
		m_time_since_update = 0;
		m_hits = 0;
		m_hit_streak = 0;
		m_age = 0;
		m_id = kf_count;
		kf_count++;
	}

	~KalmanTracker() { m_history.clear(); }

	KStateType predict();
	void update(KStateType stateMat);

	KStateType get_state();
	KStateType get_rect_xysr(float cx, float cy, float s, float r);

	KStateType lastRect;
	static int kf_count;

	int m_time_since_update;
	int m_hits;
	int m_hit_streak;
	int m_age;
	int m_id;

 private:
	void init_kf(KStateType stateMat);

	cv::KalmanFilter kf;
	cv::Mat measurement;

	std::vector<KStateType> m_history;
};
}	 // namespace vision
}	 // namespace whale
