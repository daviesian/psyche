#ifndef __FACEANALYSER_h_
#define __FACEANALYSER_h_

#include "SVR_dynamic_lin_regressors.h"
#include "SVR_static_lin_regressors.h"
#include "SVR_dynamic_lin_regressors_scale.h"

#include <string>
#include <vector>

#include <cv.h>

#include "CLM.h"

namespace Psyche
{

class FaceAnalyser{

public:


	enum RegressorType{ SVR_appearance_static_linear = 0, SVR_appearance_dynamic_linear = 1, SVR_dynamic_geom_linear = 2, SVR_combined_linear = 3};

	// Constructor from a model file (or a default one if not provided
	FaceAnalyser(std::string fname = "AU_regressors/AU_regressors.txt")
	{
		this->Read(fname);
		
		// Initialise the histograms that will represent bins from 0 - 1 (as HoG values are only stored as those)
		// Set the number of bins for the histograms
		num_bins_hog = 300;
		max_val_hog = 1;
		min_val_hog = 0;

		// The geometry histogram ranges from -3 to 3 (as it should be zero mean and unit standard dev normalised data)
		num_bins_geom = 400;
		max_val_geom = 3;
		min_val_geom = -3;

		hist_sum = 0;
		adaptation_threshold = 200;
		prediction_correction_count = 0;

		is_adapting = true;
	}

	void AddNextFrame(const cv::Mat_<uchar>& frame, const CLMTracker::CLM& clm, double timestamp_seconds);

	double GetCurrentTimeSeconds();

	double GetCurrentArousal();

	double GetCurrentValence();

	std::vector<std::pair<std::string, double>> GetCurrentAUs(bool dyn_correct = true);

	void Reset();

	double GetConfidence()
	{
		double confidence = hist_sum / 100.0;

		if(confidence > 1)
		{
			confidence = 1;
		}

		return confidence;

	}

private:

	// Private members to be used for predictions
	// The HOG descriptor of the last frame
	Mat_<double> hog_desc_frame;

	// Keep a running median of the hog descriptors
	Mat_<double> hog_desc_median;

	// Use histograms for quick (but approximate) median computation
	Mat_<unsigned int> hog_desc_hist;
	int num_bins_hog;
	double min_val_hog;
	double max_val_hog;

	// The geometry descriptor (non-rigid shape parameters from CLM)
	Mat_<double> geom_descriptor_frame;
	Mat_<double> geom_descriptor_median;

	Mat_<unsigned int> geom_desc_hist;
	int num_bins_geom;
	double min_val_geom;
	double max_val_geom;

	int hist_sum;

	// Keeping track if the model is still adapting to a face
	bool is_adapting;
	int adaptation_threshold;

	// Using the bounding box of previous analysed frame to determine if a reset is needed
	Rect_<double> face_bounding_box;
	
	void Read(std::string fname);

	void ReadRegressor(std::string fname, const vector<string>& au_names);

	// A utility function for keeping track of approximate running medians used for AU and emotion inference using a set of histograms (the histograms are evenly spaced from min_val to max_val)
	// Descriptor has to be a row vector
	void UpdateRunningMedian(cv::Mat_<unsigned int>& histogram, int& hist_sum, cv::Mat_<double>& median, const cv::Mat_<double>& descriptor, int num_bins, double min_val, double max_val);

	// The linear SVR regressors
	SVR_static_lin_regressors AU_SVR_static_appearance_lin_regressors;
	SVR_dynamic_lin_regressors AU_SVR_dynamic_appearance_lin_regressors;
	SVR_dynamic_lin_regressors_scale AU_SVR_dynamic_geom_lin_regressors;
	
	// The AUs predicted by the model are not always 0 calibrated to a person. That is don't always predict 0 for a neutral expression
	// Keeping track of the predictions we can correct for this, by assuming that at least "ratio" of frames are neutral and subtract that value of prediction, only perform the correction after min_frames
	void UpdatePredictionTrack(vector<double>& correction, const vector<pair<string, double>>& predictions, double ratio=0.25, int num_bins = 200, double min_val = 0, double max_val = 5, int min_frames = 10);

	cv::Mat_<unsigned int> prediction_correction_histogram;
	int prediction_correction_count;

	// Some dynamic scaling TODO more sciency
	vector<double> dyn_scaling;

};
  //===========================================================================
}
#endif

