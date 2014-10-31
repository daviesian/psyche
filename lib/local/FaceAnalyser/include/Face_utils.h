#ifndef __FACE_UTILS_h_
#define __FACE_UTILS_h_

#include <cv.h>
#include <highgui.h>

#include <CLM.h>

namespace Psyche
{
	//===========================================================================	
	// Defining a set of useful utility functions to be used within FaceAnalyser

	// Aligning a face to a common reference frame
	void AlignFace(cv::Mat& aligned_face, const cv::Mat& frame, const CLMTracker::CLM& clm_model, double scale = 0.6, int width = 96, int height = 96);

	void Extract_FHOG_descriptor(cv::Mat_<double>& descriptor, const cv::Mat_<uchar>& image, int cell_size = 8);

	void Visualise_FHOG(const cv::Mat_<double>& descriptor, int num_rows, int num_cols, cv::Mat& visualisation);

	// The following two methods go hand in hand
	void ExtractSummaryStatistics(const cv::Mat_<double>& descriptors, cv::Mat_<double> sum_stats);
	void AddDescriptor(cv::Mat_<double>& descriptors, cv::Mat_<double> new_descriptor, int curr_frame, int num_frames_to_keep = 60);

}
#endif
