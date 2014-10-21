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

			return latestFrame;
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

	public ref class CLMWrapper
	{
		
	};

	public ref class FaceAnalyserWrapper 
	{

	};
}
