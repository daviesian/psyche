#ifndef __FACEANALYSER_h_
#define __FACEANALYSER_h_

#include "SVR_lin_regressors.h"

#include <string>
#include <vector>

#include <cv.h>

#include "CLM.h"

namespace Psyche
{

class FaceAnalyser{

public:


	enum RegressorType{ SVR_linear = 0, MLP = 1};

	// Constructor from a model file (or a default one if not provided
	FaceAnalyser(std::string fname = "AU_regressors/AU_regressors.txt")
	{
		this->Read(fname);
		
		// Initialise the histograms that will represent bins from 0 - 1 (as HoG values are only stored as those)
		// Set the number of bins for the histograms (as it will be stored as a uchar)
		num_bins = 256;
		hist_sum = 0;
	}

	void AddNextFrame(const cv::Mat_<uchar>& frame, const CLMTracker::CLM& clm, double timestamp_seconds);

	double GetCurrentTimeSeconds();

	double GetCurrentArousal();

	double GetCurrentValence();

	std::vector<std::pair<std::string, double>> GetCurrentAUs();

	void Reset();

private:

	// Private members to be used for predictions
	// The HOG descriptor of the last frame
	Mat_<double> hog_desc_frame;

	// Keep a running median of the hog descriptors
	Mat_<double> hog_desc_median;

	// Use histograms for quick (but approximate) median computation
	Mat_<unsigned int> hog_desc_hist;
	int num_bins;
	int hist_sum;

	void Read(std::string fname);

	void ReadRegressor(std::string fname, const vector<string>& au_names);

	void ComputeRunningMedian(const cv::Mat_<double>& hog_desc);

	// The linear SVR regressors
	SVR_lin_regressors AU_SVR_lin_regressors;

	

};
  //===========================================================================
}
#endif

