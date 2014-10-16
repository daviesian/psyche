#ifndef __FACEANALYSER_h_
#define __FACEANALYSER_h_

#include <string>
#include <vector>

#include <cv.h>

#include "CLM.h"

namespace Psyche
{

class FaceAnalyser{

public:


	// A default constructor
	FaceAnalyser()
	{
	}

	// Constructor from a model file
	FaceAnalyser(std::string fname)
	{
	}
	
	void AddNextFrame(const cv::Mat_<uchar> frame, const CLMTracker::CLM& clm, double timestamp_seconds);

	double GetCurrentTime();

	double GetCurrentArousal();

	double GetCurrentValence();

	std::vector<std::pair<std::string, double>> GetCurrentAUs();

	void Reset();

private:

};
  //===========================================================================
}
#endif

