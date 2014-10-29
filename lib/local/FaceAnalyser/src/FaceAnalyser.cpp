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

	// Check if a reset is needed first
	if(face_bounding_box.area() > 0)
	{

		Rect_<double> new_bounding_box = clm.GetBoundingBox();

		// If the box overlaps do not need a reset
		double intersection_area = (face_bounding_box & new_bounding_box).area();
		double union_area = face_bounding_box.area() + new_bounding_box.area() - 2 * intersection_area;

		// If the model is already tracking what we're detecting ignore the detection, this is determined by amount of overlap
		if( intersection_area/union_area < 0.5)
		{
			this->Reset();
		}

		face_bounding_box = new_bounding_box;

	}

	// First align the face
	Mat_<uchar> aligned_face;
	AlignFace(aligned_face, frame, clm);
	
	// Extract HOG descriptor from the frame and convert it to a useable format
	Mat_<double> hog_descriptor;
	Extract_FHOG_descriptor(hog_descriptor, aligned_face);

	// Store the descriptor
	hog_desc_frame = hog_descriptor;

	int hist_count = this->hist_sum;

	UpdateRunningMedian(this->hog_desc_hist, hist_count, this->hog_desc_median, hog_descriptor, this->num_bins_hog, this->min_val_hog, this->max_val_hog);

	// Visualising the median HOG
	//Mat visualisation;
	//Psyche::Visualise_FHOG(hog_desc_median, 10, 10, visualisation);
	//cv::imshow("FHOG median", visualisation);

	// Only if geometry models are present
	if(!AU_SVR_dynamic_geom_lin_regressors.means.empty())
	{
		// Adding the geometry descriptor 
		geom_descriptor_frame = (clm.params_local.t() - AU_SVR_dynamic_geom_lin_regressors.means)/AU_SVR_dynamic_geom_lin_regressors.scaling;

		// Some clamping
		geom_descriptor_frame.setTo(Scalar(-3), geom_descriptor_frame < -3);
		geom_descriptor_frame.setTo(Scalar(3), geom_descriptor_frame > 3);

		hist_count--;
		UpdateRunningMedian(this->geom_desc_hist, hist_count, this->geom_descriptor_median, geom_descriptor_frame, this->num_bins_geom, this->min_val_geom, this->max_val_geom);

	}
	this->hist_sum = hist_count;

	if(hist_sum > adaptation_threshold)
	{
		is_adapting = false;
	}

}

// Reset the models
void FaceAnalyser::Reset()
{
	hist_sum = 0;
	is_adapting = true;

	this->hog_desc_median.setTo(Scalar(0));
	this->hog_desc_hist = Mat_<unsigned int>(hog_desc_hist.cols, hog_desc_hist.rows, (unsigned int)0);

	this->geom_descriptor_median.setTo(Scalar(0));
	this->geom_desc_hist = Mat_<unsigned int>(geom_desc_hist.cols, geom_desc_hist.rows, (unsigned int)0);

	this->prediction_correction_count = 0;
	this->prediction_correction_histogram = Mat_<unsigned int>(prediction_correction_histogram.cols, prediction_correction_histogram.rows, (unsigned int)0);
}

void FaceAnalyser::UpdateRunningMedian(cv::Mat_<unsigned int>& histogram, int& hist_count, cv::Mat_<double>& median, const cv::Mat_<double>& descriptor, int num_bins, double min_val, double max_val)
{
	// The median update
	if(histogram.empty())
	{
		histogram = Mat_<unsigned int>(descriptor.cols, num_bins, (unsigned int)0);
	}

	// Find the bins corresponding to the current descriptor
	Mat_<double> converted_descriptor = (descriptor - min_val)*((double)num_bins)/(max_val - min_val);

	// Capping the top and bottom values
	converted_descriptor.setTo(Scalar(num_bins-1), converted_descriptor > num_bins - 1);
	converted_descriptor.setTo(Scalar(0), converted_descriptor < 0);
	
	for(int i = 0; i < histogram.rows; ++i)
	{
		int index = (int)converted_descriptor.at<double>(i);
		histogram.at<unsigned int>(i, index)++;
	}

	// Update the histogram count
	hist_count++;

	if(hist_count == 1)
	{
		median = descriptor.clone();
	}
	else
	{
		// Recompute the median
		int cutoff_point = (hist_count + 1)/2;

		// For each dimension
		for(int i = 0; i < histogram.rows; ++i)
		{
			int cummulative_sum = 0;
			for(int j = 0; j < histogram.cols; ++j)
			{
				cummulative_sum += histogram.at<unsigned int>(i, j);
				if(cummulative_sum > cutoff_point)
				{
					median.at<double>(i) = min_val + j * (max_val/num_bins) + (0.5*(max_val-min_val)/num_bins);
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
		vector<string> svr_lin_stat_aus;
		vector<double> svr_lin_stat_preds;

		AU_SVR_static_appearance_lin_regressors.Predict(svr_lin_stat_preds, svr_lin_stat_aus, hog_desc_frame);

		for(size_t i = 0; i < svr_lin_stat_preds.size(); ++i)
		{
			predictions.push_back(pair<string, double>(svr_lin_stat_aus[i], svr_lin_stat_preds[i]));
		}

		vector<string> svr_lin_dyn_aus;
		vector<double> svr_lin_dyn_preds;

		AU_SVR_dynamic_appearance_lin_regressors.Predict(svr_lin_dyn_preds, svr_lin_dyn_aus, hog_desc_frame, this->hog_desc_median);

		for(size_t i = 0; i < svr_lin_dyn_preds.size(); ++i)
		{
			predictions.push_back(pair<string, double>(svr_lin_dyn_aus[i], svr_lin_dyn_preds[i]));
		}

		vector<string> svr_lin_dyn_geom_aus;
		vector<double> svr_lin_dyn_geom_preds;

		AU_SVR_dynamic_geom_lin_regressors.Predict(svr_lin_dyn_geom_preds, svr_lin_dyn_geom_aus, this->geom_descriptor_frame, this->geom_descriptor_median);

		for(size_t i = 0; i < svr_lin_dyn_geom_aus.size(); ++i)
		{
			predictions.push_back(pair<string, double>(svr_lin_dyn_geom_aus[i], svr_lin_dyn_geom_preds[i]));
		}

		vector<double> correction;
		UpdatePredictionTrack(correction, predictions);

		for(size_t i = 0; i < predictions.size(); ++i)
		{
			predictions[i].second = predictions[i].second - correction[i];
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

void FaceAnalyser::UpdatePredictionTrack(vector<double>& correction, const vector<pair<string, double>>& predictions, double ratio, int num_bins, double min_val, double max_val, int min_frames)
{
		// The median update
	if(prediction_correction_histogram.empty())
	{
		prediction_correction_histogram = Mat_<unsigned int>(predictions.size(), num_bins, (unsigned int)0);
	}
	
	for(int i = 0; i < prediction_correction_histogram.rows; ++i)
	{
		// Find the bins corresponding to the current descriptor
		int index = (predictions[i].second - min_val)*((double)num_bins)/(max_val - min_val);
		if(index < 0)
		{
			index = 0;
		}
		else if(index > num_bins - 1)
		{
			index = num_bins - 1;
		}
		prediction_correction_histogram.at<unsigned int>(i, index)++;
	}

	// Update the histogram count
	prediction_correction_count++;

	if(prediction_correction_count >= min_frames)
	{
		// Recompute the correction
		int cutoff_point = ratio * prediction_correction_count;

		// For each dimension
		for(int i = 0; i < prediction_correction_histogram.rows; ++i)
		{
			int cummulative_sum = 0;
			for(int j = 0; j < prediction_correction_histogram.cols; ++j)
			{
				cummulative_sum += prediction_correction_histogram.at<unsigned int>(i, j);
				if(cummulative_sum > cutoff_point)
				{
					double corr = min_val + j * (max_val/num_bins);
					correction.push_back(corr);
					break;
				}
			}
		}
	}
	else
	{
		correction.resize(predictions.size(), 0);
	}
}

void FaceAnalyser::ReadRegressor(std::string fname, const vector<string>& au_names)
{
	ifstream regressor_stream(fname.c_str(), ios::in | ios::binary);

	// First read the input type
	int regressor_type;
	regressor_stream.read((char*)&regressor_type, 4);

	if(regressor_type == SVR_appearance_static_linear)
	{
		AU_SVR_static_appearance_lin_regressors.Read(regressor_stream, au_names);		
	}
	else if(regressor_type == SVR_appearance_dynamic_linear)
	{
		AU_SVR_dynamic_appearance_lin_regressors.Read(regressor_stream, au_names);		
	}
	else if(regressor_type == SVR_dynamic_geom_linear)
	{
		AU_SVR_dynamic_geom_lin_regressors.Read(regressor_stream, au_names);		
	}

}