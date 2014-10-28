#include <Face_utils.h>

#include <CLM_utils.h>

// For FHOG visualisation
#include <dlib/gui_widgets.h>

using namespace cv;
using namespace std;

namespace Psyche
{
	// Aligning a face to a common reference frame
	void AlignFace(cv::Mat& aligned_face, const cv::Mat& frame, const CLMTracker::CLM& clm_model, double sim_scale, int out_width, int out_height)
	{
		// Will warp to scaled mean shape
		Mat_<double> similarity_normalised_shape = clm_model.pdm.mean_shape * sim_scale;
	
		// Discard the z component
		similarity_normalised_shape = similarity_normalised_shape(Rect(0, 0, 1, 2*similarity_normalised_shape.rows/3)).clone();

		Mat_<double> source_landmarks = clm_model.detected_landmarks.reshape(1, 2).t();
		Mat_<double> destination_landmarks = similarity_normalised_shape.reshape(1, 2).t();

		Matx22d scale_rot_matrix = CLMTracker::AlignShapesWithScale(source_landmarks, destination_landmarks);
		Matx23d warp_matrix;

		warp_matrix(0,0) = scale_rot_matrix(0,0);
		warp_matrix(0,1) = scale_rot_matrix(0,1);
		warp_matrix(1,0) = scale_rot_matrix(1,0);
		warp_matrix(1,1) = scale_rot_matrix(1,1);

		double tx = clm_model.params_global[4];
		double ty = clm_model.params_global[5];

		Vec2d T(tx, ty);
		T = scale_rot_matrix * T;

		// Make sure centering is correct
		warp_matrix(0,2) = -T(0) + out_width/2;
		warp_matrix(1,2) = -T(1) + out_height/2;

		cv::warpAffine(frame, aligned_face, warp_matrix, Size(out_width, out_height), INTER_LINEAR);
	}

	void Visualise_FHOG(const cv::Mat_<double>& descriptor, int num_rows, int num_cols, cv::Mat& visualisation)
	{

		// First convert to dlib format
		dlib::array2d<dlib::matrix<float,31,1> > hog(num_rows, num_cols);
		
		cv::MatConstIterator_<double> descriptor_it = descriptor.begin();
		for(int y = 0; y < num_cols; ++y)
		{
			for(int x = 0; x < num_rows; ++x)
			{
				for(unsigned int o = 0; o < 31; ++o)
				{
					hog[y][x](o) = *descriptor_it++;
				}
			}
		}

		// Draw the FHOG to OpenCV format
		auto fhog_vis = dlib::draw_fhog(hog);
		visualisation = dlib::toMat(fhog_vis).clone();
	}

	// Create a row vector Felzenszwalb HOG descriptor from a given image
	void Extract_FHOG_descriptor(cv::Mat_<double>& descriptor, const cv::Mat_<uchar>& image, int cell_size)
	{
		dlib::cv_image<uchar> dlib_warped_img(image);

		dlib::array2d<dlib::matrix<float,31,1> > hog;
		dlib::extract_fhog_features(dlib_warped_img, hog, cell_size);

		// Convert to a usable format
		int num_cols = hog.nc();
		int num_rows = hog.nr();

		descriptor = Mat_<double>(1, num_cols * num_rows * 31);
		cv::MatIterator_<double> descriptor_it = descriptor.begin();
		for(int y = 0; y < num_cols; ++y)
		{
			for(int x = 0; x < num_rows; ++x)
			{
				for(unsigned int o = 0; o < 31; ++o)
				{
					*descriptor_it++ = (double)hog[y][x](o);
				}
			}
		}
	}
}