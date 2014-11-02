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

	if(!clm.detection_success)
	{
		this->Reset();
	}

	// First align the face
	Mat_<uchar> aligned_face;
	AlignFace(aligned_face, frame, clm);
	
	//imshow("Aligned face", aligned_face);

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

	this->hist_sum = hist_count;

	if(hist_sum > adaptation_threshold)
	{
		is_adapting = false;
	}

	// Perform AU prediction
	AU_predictions = PredictCurrentAUs(true);

	// Perform AV predictions
	PredictCurrentAVs(clm);

	this->current_time_seconds = timestamp_seconds;
}

void FaceAnalyser::PredictCurrentAVs(const CLMTracker::CLM& clm)
{
	// Can update the AU prediction track (used for predicting emotions)
	// Pick out the predictions
	Mat_<double> preds(1, AU_predictions.size(), 0.0);
	for( size_t i = 0; i < AU_predictions.size(); ++i)
	{
		preds.at<double>(0, i) = AU_predictions[i].second;
	}

	AddDescriptor(AU_prediction_track, preds, hist_sum - 1, 60);
	Mat_<double> sum_stats_AU;
	ExtractSummaryStatistics(AU_prediction_track, sum_stats_AU);
	
	vector<string> names_v;
	vector<double> prediction_v;
	valence_predictor_lin_geom.Predict(prediction_v, names_v, sum_stats_AU);
	double valence_tmp = prediction_v[0];

	// Arousal prediction
	// Adding the geometry descriptor 
	Mat_<double> geom_params;

	// This is for tracking median of geometry parameters to subtract from the other models
	Vec3d g_params(clm.params_global[1], clm.params_global[2], clm.params_global[3]);
	geom_params.push_back(Mat(g_params));
	geom_params.push_back(clm.params_local);
	geom_params = geom_params.t();

	AddDescriptor(geom_desc_track, geom_params, hist_sum - 1);
	Mat_<double> sum_stats_geom;
	ExtractSummaryStatistics(geom_desc_track, sum_stats_geom);

	sum_stats_geom = (sum_stats_geom - arousal_predictor_lin_geom.means)/arousal_predictor_lin_geom.scaling;

	// Some clamping
	sum_stats_geom.setTo(Scalar(-5), sum_stats_geom < -5);
	sum_stats_geom.setTo(Scalar(5), sum_stats_geom > 5);

	vector<string> names;
	vector<double> prediction;
	arousal_predictor_lin_geom.Predict(prediction, names, sum_stats_geom);
	double arousal_tmp = prediction[0];

	vector<double> correction(2, 0.0);
	vector<pair<string,double>> predictions;
	predictions.push_back(pair<string,double>("arousal", arousal_tmp));
	predictions.push_back(pair<string,double>("valence", valence_tmp));

	//UpdatePredictionTrack(av_prediction_correction_histogram, av_prediction_correction_count, correction, predictions, 0.5, 200, -1.0, 1.0, frames_for_adaptation);

	//cout << "Corrections: ";
	//for(size_t i = 0; i < correction.size(); ++i)
	//{
		//predictions[i].second = predictions[i].second - correction[i];
		//cout << correction[i] << " ";
	//}
	//cout << endl;

	// Correction of AU and Valence values (scale them for better visibility) (manual for now)
	predictions[0].second = predictions[0].second + 0.1;

	if(predictions[0].second > 0)
	{
		predictions[0].second = predictions[0].second * 2;
	}

	if(predictions[1].second > 0)
	{
		predictions[1].second = predictions[1].second * 3;
	}
	else
	{
		predictions[1].second = predictions[1].second * 10;
	}

	this->arousal_value = predictions[0].second;
	this->valence_value = predictions[1].second;

}

// Reset the models
void FaceAnalyser::Reset()
{
	hist_sum = 0;
	is_adapting = true;

	this->hog_desc_median.setTo(Scalar(0));
	this->hog_desc_hist = Mat_<unsigned int>(hog_desc_hist.rows, hog_desc_hist.cols, (unsigned int)0);

	this->geom_descriptor_median.setTo(Scalar(0));
	this->geom_desc_hist = Mat_<unsigned int>(geom_desc_hist.rows, geom_desc_hist.cols, (unsigned int)0);

	// Reset the predictions
	AU_prediction_track = Mat_<double>(AU_prediction_track.rows, AU_prediction_track.cols, 0.0);

	geom_desc_track = Mat_<double>(geom_desc_track.rows, geom_desc_track.cols, 0.0);

	arousal_value = 0.0;
	valence_value = 0.0;

	// 0 callibration predictions
	this->au_prediction_correction_count = 0;
	this->au_prediction_correction_histogram = Mat_<unsigned int>(au_prediction_correction_histogram.rows, au_prediction_correction_histogram.cols, (unsigned int)0);

	this->av_prediction_correction_count = 0;
	this->av_prediction_correction_histogram = Mat_<unsigned int>(av_prediction_correction_histogram.rows, av_prediction_correction_histogram.cols, (unsigned int)0);

	dyn_scaling = vector<double>(dyn_scaling.size(), 5.0);	

}

void FaceAnalyser::UpdateRunningMedian(cv::Mat_<unsigned int>& histogram, int& hist_count, cv::Mat_<double>& median, const cv::Mat_<double>& descriptor, int num_bins, double min_val, double max_val)
{

	double length = max_val - min_val;
	if(length < 0)
		length = -length;

	// The median update
	if(histogram.empty())
	{
		histogram = Mat_<unsigned int>(descriptor.cols, num_bins, (unsigned int)0);
	}

	// Find the bins corresponding to the current descriptor
	Mat_<double> converted_descriptor = (descriptor - min_val)*((double)num_bins)/(length);

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
					median.at<double>(i) = min_val + j * (max_val/num_bins) + (0.5*(length)/num_bins);
					break;
				}
			}
		}
	}
}

// Apply the current predictors to the currently stored descriptors
vector<pair<string, double>> FaceAnalyser::PredictCurrentAUs(bool dyn_correct)
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

		// Correction that drags the predicion to 0
		vector<double> correction(predictions.size(), 0.0);
		UpdatePredictionTrack(au_prediction_correction_histogram, au_prediction_correction_count, correction, predictions, 0.25, 200, 0, 5, frames_for_adaptation);

		for(size_t i = 0; i < correction.size(); ++i)
		{
			predictions[i].second = predictions[i].second - correction[i];

			if(predictions[i].second < 0)
			{
				predictions[i].second = 0;
			}
		}

		if(dyn_correct)
		{
			// Some scaling for effect (not really scientific though)
			// TODO this is ad-hoc, not neat, but nice for demos
			if(dyn_scaling.empty())
			{
				dyn_scaling = vector<double>(predictions.size(), 5.0);
			}
		
			for(size_t i = 0; i < predictions.size(); ++i)
			{
				// First establish presence (assume it is maximum as we have not seen max) TODO this could be more robust
				if(predictions[i].second > 1)
				{
					double scaling_curr = 5.0 / predictions[i].second;
				
					if(scaling_curr < dyn_scaling[i])
					{
						dyn_scaling[i] = scaling_curr;
					}
					predictions[i].second = predictions[i].second * dyn_scaling[i];
				}

				if(predictions[i].second > 5)
				{
					predictions[i].second = 5;
				}
			}
		}
	}

	return predictions;
}

vector<pair<string, double>> FaceAnalyser::GetCurrentAUs()
{
	return AU_predictions;
}

// Reading in AU prediction modules
void FaceAnalyser::ReadAU(std::string au_model_location)
{

	// Open the list of the regressors in the file
	ifstream locations(au_model_location.c_str(), ios::in);

	if(!locations.is_open())
	{
		cout << "Couldn't open the AU prediction files aborting" << endl;
		cout.flush();
		return;
	}

	string line;
	
	// The other module locations should be defined as relative paths from the main model
	boost::filesystem::path root = boost::filesystem::path(au_model_location).parent_path();		
	
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

// Reading in AU prediction modules
void FaceAnalyser::ReadAV(std::string av_model_location)
{

	// Open the list of the regressors in the file
	ifstream locations(av_model_location.c_str(), ios::in);

	if(!locations.is_open())
	{
		cout << "Couldn't open the AV prediction files aborting" << endl;
		cout.flush();
		return;
	}

	string line;
	
	// The other module locations should be defined as relative paths from the main model
	boost::filesystem::path root = boost::filesystem::path(av_model_location).parent_path();		
	
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

		// append the lovstion to root location (boost syntax)
		location = (root / location).string();
				
		if(strcmp(name.c_str(), "arousal") == 0)
		{
			ifstream regressor_stream(location.c_str(), ios::in | ios::binary);

			// First read the input type
			int regressor_type;
			regressor_stream.read((char*)&regressor_type, 4);
			assert(regressor_type == SVR_dynamic_geom_linear);
			
			vector<string> names;
			names.push_back("arousal");

			arousal_predictor_lin_geom.Read(regressor_stream, names);
		}
		if(strcmp(name.c_str(), "valence") == 0)
		{
			ifstream regressor_stream(location.c_str(), ios::in | ios::binary);

			// First read the input type
			int regressor_type;
			regressor_stream.read((char*)&regressor_type, 4);
			assert(regressor_type == SVR_dynamic_geom_linear);
			
			vector<string> names;
			names.push_back("arousal");
			valence_predictor_lin_geom.Read(regressor_stream, names);
		}
		
	}
  
}

void FaceAnalyser::UpdatePredictionTrack(Mat_<unsigned int>& prediction_corr_histogram, int& prediction_correction_count, vector<double>& correction, const vector<pair<string, double>>& predictions, double ratio, int num_bins, double min_val, double max_val, int min_frames)
{
	double length = max_val - min_val;
	if(length < 0)
		length = -length;

	correction.resize(predictions.size(), 0);

	// The median update
	if(prediction_corr_histogram.empty())
	{
		prediction_corr_histogram = Mat_<unsigned int>(predictions.size(), num_bins, (unsigned int)0);
	}
	
	for(int i = 0; i < prediction_corr_histogram.rows; ++i)
	{
		// Find the bins corresponding to the current descriptor
		int index = (predictions[i].second - min_val)*((double)num_bins)/(length);
		if(index < 0)
		{
			index = 0;
		}
		else if(index > num_bins - 1)
		{
			index = num_bins - 1;
		}
		prediction_corr_histogram.at<unsigned int>(i, index)++;
	}

	// Update the histogram count
	prediction_correction_count++;

	if(prediction_correction_count >= min_frames)
	{
		// Recompute the correction
		int cutoff_point = ratio * prediction_correction_count;

		// For each dimension
		for(int i = 0; i < prediction_corr_histogram.rows; ++i)
		{
			int cummulative_sum = 0;
			for(int j = 0; j < prediction_corr_histogram.cols; ++j)
			{
				cummulative_sum += prediction_corr_histogram.at<unsigned int>(i, j);
				if(cummulative_sum > cutoff_point)
				{
					double corr = min_val + j * (length/num_bins);
					correction[i] = corr;
					break;
				}
			}
		}
	}
}

void FaceAnalyser::GetSampleHist(Mat_<unsigned int>& prediction_corr_histogram, int prediction_correction_count, vector<double>& sample, double ratio, int num_bins, double min_val, double max_val)
{

	double length = max_val - min_val;
	if(length < 0)
		length = -length;

	sample.resize(prediction_corr_histogram.rows, 0);

	// Recompute the correction
	int cutoff_point = ratio * prediction_correction_count;

	// For each dimension
	for(int i = 0; i < prediction_corr_histogram.rows; ++i)
	{
		int cummulative_sum = 0;
		for(int j = 0; j < prediction_corr_histogram.cols; ++j)
		{
			cummulative_sum += prediction_corr_histogram.at<unsigned int>(i, j);
			if(cummulative_sum > cutoff_point)
			{
				double corr = min_val + j * (length/num_bins);
				sample[i] = corr;
				break;
			}
		}
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

}

double FaceAnalyser::GetCurrentArousal() {
	return arousal_value * 2;
	//return sin(current_time_seconds * 1);
}

double FaceAnalyser::GetCurrentValence() {
	return valence_value * 2;
	//return cos(current_time_seconds * 1.2);
}

double FaceAnalyser::GetCurrentTimeSeconds() {
	return current_time_seconds;
}