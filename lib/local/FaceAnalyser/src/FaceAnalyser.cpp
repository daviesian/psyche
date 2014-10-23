#include "FaceAnalyser.h"

#include "Face_utils.h"

#include <stdio.h>
#include <iostream>

#include <string>

#include <filesystem.hpp>
#include <filesystem/fstream.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace Psyche;

using namespace std;

void FaceAnalyser::AddNextFrame(const cv::Mat_<uchar>& frame, const CLMTracker::CLM& clm, double timestamp_seconds)
{
	// First align the face
	Mat_<uchar> aligned_face;
	AlignFace(aligned_face, frame, clm);
	
	// Extract HOG descriptor from the frame and convert it to a useable format
	Mat_<double> hog_descriptor;
	Extract_FHOG_descriptor(hog_descriptor, aligned_face);

	// Store the descriptor
	hog_desc_frame = hog_descriptor;

	ComputeRunningMedian(hog_descriptor);
}

void FaceAnalyser::ComputeRunningMedian(const cv::Mat_<double>& hog_descriptor)
{
	// The median update
	if(hog_desc_hist.empty())
	{
		hog_desc_hist = Mat_<unsigned int>(hog_desc_frame.cols, this->num_bins, 0.0);
	}

	// Find the bins corresponding to the current descriptor
	Mat_<uchar> hog_desc_bin;
	hog_descriptor.convertTo(hog_desc_bin, hog_desc_bin.type(), num_bins);

	// TODO rem
	//cout << hog_descriptor(Rect(0,0,20,1)) << endl;
	//cout << hog_desc_bin(Rect(0,0,20,1)) << endl;

	for(int i = 0; i < hog_desc_hist.rows; ++i)
	{
		hog_desc_hist.at<unsigned int>(i, hog_desc_bin.at<uchar>(i))++;
	}

	// Update the histogram sum
	hist_sum++;

	if(hist_sum == 1)
	{
		hog_desc_median = hog_descriptor.clone();
	}
	else
	{
		// Recompute the median
		int cutoff_point = (hist_sum+1)/2;

		// For each dimension
		for(int i = 0; i < hog_desc_hist.rows; ++i)
		{
			int cummulative_sum = 0;
			for(int j = 0; j < hog_desc_hist.cols; ++j)
			{
				cummulative_sum += hog_desc_hist.at<unsigned int>(i, j);
				if(cummulative_sum > cutoff_point)
				{
					hog_desc_median.at<double>(i) = j * (1.0/num_bins) + (0.5/num_bins);
					break;
				}
			}
		}
	}
}

// Apply the current predictors to the currently stored descriptors
vector<pair<string, double>> FaceAnalyser::GetCurrentAUs()
{

	vector<pair<string, double>> predictions;

	if(!hog_desc_frame.empty())
	{
		vector<string> svr_lin_aus;
		vector<double> svr_lin_preds;

		AU_SVR_lin_regressors.Predict(svr_lin_preds, svr_lin_aus, hog_desc_frame);

		for(int i = 0; i < svr_lin_preds.size(); ++i)
		{
			predictions.push_back(pair<string, double>(svr_lin_aus[i], svr_lin_preds[i]));
		}
	}

	return predictions;
}

// The main constructor of the face analyser using 
void FaceAnalyser::Read(std::string face_analyser_loc)
{

	// Open the list of the regressors in the file
	ifstream locations(face_analyser_loc.c_str(), ios::in);

	if(!locations.is_open())
	{
		cout << "Couldn't open the AU prediction files aborting" << endl;
		cout.flush();
		return;
	}

	string line;
	
	// The other module locations should be defined as relative paths from the main model
	boost::filesystem::path root = boost::filesystem::path(face_analyser_loc).parent_path();		
	
	// The main file contains the references to other files
	while (!locations.eof())
	{ 
		
		getline(locations, line);

		stringstream lineStream(line);

		string name;
		string location;

		// figure out which module is to be read from which file
		lineStream >> location;

		// Parse comma separated names that this regressor produces
		name = lineStream.str();
		int index = name.find_first_of(' ');

		if(index >= 0)
		{
			name = name.substr(index+1);
			
			// remove carriage return at the end for compatibility with unix systems
			if(name.size() > 0 && name.at(name.size()-1) == '\r')
			{
				name = name.substr(0, location.size()-1);
			}
		}
		vector<string> au_names;
		boost::split(au_names, name, boost::is_any_of(","));

		// append the lovstion to root location (boost syntax)
		location = (root / location).string();
				
		ReadRegressor(location, au_names);
	}
  
}

void FaceAnalyser::ReadRegressor(std::string fname, const vector<string>& au_names)
{
	ifstream regressor_stream(fname.c_str(), ios::in | ios::binary);

	// First read the input type
	int regressor_type;
	regressor_stream.read((char*)&regressor_type, 4);

	if(regressor_type == SVR_linear)
	{
		AU_SVR_lin_regressors.Read(regressor_stream, au_names);		
	}

}