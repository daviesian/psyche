// PsycheInterop.h

#pragma once

#pragma unmanaged

// Include all the unmanaged things we need.

#include "cv.h"
#include "highgui.h"

#pragma managed

#include <CLM.h>
#include <CLMTracker.h>
#include <CLMParameters.h>
#include <CLM_utils.h>

#include <FaceAnalyser.h>

using namespace System;
using namespace OpenCVWrappers;

namespace PsycheInterop {

	public ref class Capture
	{
	private:

		VideoCapture* vc;

		RawImage^ latestFrame;
		RawImage^ mirroredFrame;
		RawImage^ grayFrame;

	public:
		
		Capture(int device)
		{
			latestFrame = gcnew RawImage();
			mirroredFrame = gcnew RawImage();

			vc = new VideoCapture(device);
		}

		RawImage^ GetNextFrame()
		{
			*vc >> (mirroredFrame->Mat);

			flip(mirroredFrame->Mat, latestFrame->Mat, 1);

			if (grayFrame == nullptr) {
				if (latestFrame->Width > 0) {
					grayFrame = gcnew RawImage(latestFrame->Width, latestFrame->Height, CV_8UC1);
				}
			}

			if (grayFrame != nullptr) {
				cvtColor(latestFrame->Mat, grayFrame->Mat, CV_BGR2GRAY);
			}

			return latestFrame;
		}

		RawImage^ GetCurrentFrameGray() {
			return grayFrame;
		}
		
		// Finalizer. Definitely called before Garbage Collection,
		// but not automatically called on explicit Dispose().
		// May be called multiple times.
		!Capture()
		{
			delete vc; // Automatically closes capture object before freeing memory.
		}

		// Destructor. Called on explicit Dispose() only.
		~Capture()
		{
			this->!Capture();
		}
	};

	public ref class Vec6d {

	private:
		cv::Vec6d* vec;

	public:

		Vec6d(cv::Vec6d vec): vec(new cv::Vec6d(vec)) { }
		
		cv::Vec6d* getVec() { return vec; }
	};

	namespace CLMTracker {

		public ref class CLMParameters
		{
		private:
			::CLMTracker::CLMParameters* params;

		public:

			CLMParameters() : params(new ::CLMTracker::CLMParameters()) { }

			::CLMTracker::CLMParameters* getParams() {
				return params;
			}
		};

		public ref class CLM
		{
		private:

			::CLMTracker::CLM* clm;

		public:

			CLM() : clm(new ::CLMTracker::CLM()) { }

			::CLMTracker::CLM* getCLM() {
				return clm;
			}

			void Reset() {
				clm->Reset();
			}

			void Reset(double x, double y) {
				clm->Reset(x, y);
			}


			bool DetectLandmarksInVideo(RawImage^ image, CLMParameters^ clmParams) {
				return ::CLMTracker::DetectLandmarksInVideo(image->Mat, *clm, *clmParams->getParams());
			}

			Vec6d^ GetCorrectedPoseCamera(double fx, double fy, double cx, double cy, CLMParameters^ clmParams) {
				return gcnew Vec6d(::CLMTracker::GetCorrectedPoseCamera(*clm, fx, fy, cx, cy, *clmParams->getParams()));
			}

			Vec6d^ GetCorrectedPoseCameraPlane(double fx, double fy, double cx, double cy, CLMParameters^ clmParams) {
				return gcnew Vec6d(::CLMTracker::GetCorrectedPoseCameraPlane(*clm, fx, fy, cx, cy, *clmParams->getParams()));
			}
	
			// Static functions from the CLMTracker namespace.
			void Draw(RawImage^ img) {
				::CLMTracker::Draw(img->Mat, *clm);
			}

			void DrawBox(RawImage^ image, double r, double g, double b, int thickness, float fx, float fy, float cx, float cy) {
				cv::Scalar color = cv::Scalar(r,g,b,1);
				::CLMTracker::CLMParameters params = ::CLMTracker::CLMParameters();
				cv::Vec6d pose = ::CLMTracker::GetCorrectedPoseCameraPlane(*clm, fx,fy, cx, cy, params);

				::CLMTracker::DrawBox(image->Mat, pose, color, thickness, fx, fy, cx, cy);
			}


		};

	}


	public ref class FaceAnalyser
	{
	private:
		Psyche::FaceAnalyser* faceAnalyser;

	public:

		FaceAnalyser() {
			faceAnalyser = new Psyche::FaceAnalyser();
		}

		void AddNextFrame(RawImage^ frame, CLMTracker::CLM^ clm, double timestamp_seconds) {
			faceAnalyser->AddNextFrame(frame->Mat, *clm->getCLM(), timestamp_seconds);
		}

		double GetCurrentTimeSeconds() {
			return faceAnalyser->GetCurrentTimeSeconds();
		}

		double GetCurrentArousal() {
			return faceAnalyser->GetCurrentArousal();
		}

		double GetCurrentValence() {
			return faceAnalyser->GetCurrentValence();
		}

		System::Collections::Generic::Dictionary<System::String^, double>^ GetCurrentAUs() {
			std::vector<std::pair<std::string, double>> aus = faceAnalyser->GetCurrentAUs();

			System::Collections::Generic::Dictionary<System::String^, double>^ dict = gcnew System::Collections::Generic::Dictionary<System::String^, double>();

			for(std::pair<std::string, double> p : aus) {
				dict->Add(gcnew System::String(p.first.c_str()), p.second);
			}
			return dict;
		}

		void Reset() {
			faceAnalyser->Reset();
		}

		double GetConfidence() {
			return faceAnalyser->GetConfidence();
		}
	};
}
