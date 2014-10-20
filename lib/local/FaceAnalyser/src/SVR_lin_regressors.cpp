#include "SVR_lin_regressors.h"

#include "CLM_utils.h"

using namespace Psyche;

void SVR_lin_regressors::Read(std::ifstream& stream, const std::vector<std::string>& au_names)
{

	CLMTracker::ReadMatBin(stream, this->means);

	Mat_<double> support_vectors_curr;
	CLMTracker::ReadMatBin(stream, support_vectors_curr);

	double bias;
	stream.read((char *)&bias, 8);

	this->support_vectors.push_back(support_vectors_curr);
	this->biases.push_back(cv::Mat_<double>(1, 1, bias));

	for(size_t i=0; i < au_names.size(); ++i)
	{
		this->AU_names.push_back(au_names[i]);
	}
}
