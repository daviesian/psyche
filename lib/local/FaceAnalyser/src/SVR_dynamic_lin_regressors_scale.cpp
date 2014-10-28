#include "SVR_dynamic_lin_regressors_scale.h"

#include "CLM_utils.h"

using namespace Psyche;

void SVR_dynamic_lin_regressors_scale::Read(std::ifstream& stream, const std::vector<std::string>& au_names)
{
	// TODO check if means are the same. TODO sth wrong if not
	CLMTracker::ReadMatBin(stream, this->means);
	CLMTracker::ReadMatBin(stream, this->scaling);

	Mat_<double> support_vectors_curr;
	CLMTracker::ReadMatBin(stream, support_vectors_curr);

	double bias;
	stream.read((char *)&bias, 8);

	// Add a column vector to the matrix of support vectors (each column is a support vector)
	if(!this->support_vectors.empty())
	{	
		cv::transpose(this->support_vectors, this->support_vectors);
		cv::transpose(support_vectors_curr, support_vectors_curr);
		this->support_vectors.push_back(support_vectors_curr);
		cv::transpose(this->support_vectors, this->support_vectors);

		cv::transpose(this->biases, this->biases);
		this->biases.push_back(cv::Mat_<double>(1, 1, bias));
		cv::transpose(this->biases, this->biases);

	}
	else
	{
		this->support_vectors.push_back(support_vectors_curr);
		this->biases.push_back(cv::Mat_<double>(1, 1, bias));
	}
	
	for(size_t i=0; i < au_names.size(); ++i)
	{
		this->AU_names.push_back(au_names[i]);
	}
}

// Prediction using the descriptor and median that have already been scaled
void SVR_dynamic_lin_regressors_scale::Predict(std::vector<double>& predictions, std::vector<std::string>& names, const cv::Mat_<double>& descriptor, const cv::Mat_<double>& running_median)
{
	if(AU_names.size() > 0)
	{
		Mat_<double> preds = (descriptor - running_median) * this->support_vectors + this->biases;

		// Remove below 0 and above 5 predictions
		preds.setTo(0, preds < 0);
		preds.setTo(5, preds > 5);

		for(MatIterator_<double> pred_it = preds.begin(); pred_it != preds.end(); ++pred_it)
		{		
			predictions.push_back(*pred_it);
		}

		names = this->AU_names;
	}
}