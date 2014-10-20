#include "FaceAnalyser.h"

#include <stdio.h>
#include <iostream>

#include <string>

#include <filesystem.hpp>
#include <filesystem/fstream.hpp>

using namespace Psyche;

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
	
	vector<string> AU_predictor_locations;
	vector<string> AU_names;

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
		name = lineStream.str();
		
		if(name.size() > 0)
			name.erase(location.begin()); // remove the first space
				
		// remove carriage return at the end for compatibility with unix systems
		if(name.size() > 0 && name.at(location.size()-1) == '\r')
		{
			name = name.substr(0, location.size()-1);
		}

		// append the lovstion to root location (boost syntax)
		location = (root / location).string();
				
		AU_predictor_locations.push_back(location);
		AU_names.push_back(name);
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