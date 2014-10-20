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

	// A default constructor
	FaceAnalyser(){}

	// Constructor from a model file
	FaceAnalyser(std::string fname)
	{
		this->Read(fname);
	}
	
	void AddNextFrame(const cv::Mat_<uchar> frame, const CLMTracker::CLM& clm, double timestamp_seconds);

	double GetCurrentTime();

	double GetCurrentArousal();

	double GetCurrentValence();

	std::vector<std::pair<std::string, double>> GetCurrentAUs();

	void Reset();

private:

	void Read(std::string fname);

	void ReadRegressor(std::string fname, const vector<string>& au_names);

	// The linear SVR regressors
	SVR_lin_regressors AU_SVR_lin_regressors;

};
  //===========================================================================
}
#endif

