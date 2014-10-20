#ifndef __SVRLINREGRESSORS_h_
#define __SVRLINREGRESSORS_h_

#include <vector>
#include <string>

#include <stdio.h>
#include <iostream>

#include <dlib\array2d.h>

#include <cv.h>

namespace Psyche
{

// Collection of linear SVR regressors for AU prediction
class SVR_lin_regressors{

public:

	SVR_lin_regressors()
	{}

	// Predict the AU from HOG appearance of the face
	double Predict(std::vector<double>& predictions, std::vector<std::string>& names, dlib::array2d<dlib::matrix<float,31,1> >* hog);

	// Reading in the model (or adding to it)
	void Read(std::ifstream& stream, const std::vector<std::string>& au_names);

private:

	// The names of Action Units this model is responsible for
	std::vector<std::string> AU_names;

	// For normalisation
	cv::Mat_<float> means;
	
	// For actual prediction
	cv::Mat_<float> support_vectors;	
	cv::Mat_<float> biases;

};
  //===========================================================================
}
#endif

