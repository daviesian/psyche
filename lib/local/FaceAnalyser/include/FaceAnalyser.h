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

	void Read(std::string fname);

	void ReadRegressor(std::string fname, const vector<string>& au_names);

	// The linear SVR regressors
	SVR_lin_regressors AU_SVR_lin_regressors;

	

};
  //===========================================================================
}
#endif

