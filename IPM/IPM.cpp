#include "stdafx.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <fstream>
#include <sstream>
#include <opencv2/opencv.hpp>
#include "Common.h"
#include "Calibration.h"
#include "MapPair.h"

using namespace std;
using namespace cv;


int main(int argc, char* argv[])
{
	if (argc != arguments) {
		string usageError = " <imagefilename> <datafilename>";
		return helpful(argv[programname], usageError);
	}

	//load input image into source mat
	Mat src = imread(argv[1], 1);
	if (!src.data)
	{
		printf("No image data \n");
		return noimageerror;
	}

	//read data file and store in data vector
	ifstream fin(argv[2]);
	if (!fin.is_open()) {
		std::cerr << "There was a problem opening the data file.\n";
		return baddatafile;
	}
	vector<vector<double>> data = readdata(fin);
	

	//dimensions of image, which is 4000 x 3010 pixels
	int m = src.cols, n = src.rows;

	//predetermined cx cy cz camera distance from origin, measured in field test (from data file)
	double cx = data[0][0], cy = data[0][1], cz = data[0][2];

	//horizontal and vertical FOV divided by 2, if given by manufacturer (from data file)
	double alpha_h = (data[1][0] / 2)*M_PI / 180;
	double alpha_v = (data[1][1] / 2)*M_PI / 180;

	//xy and uv lookup tables (from data file)
	int points = data[2].size();
	double ** xy = new double*[2];
	double ** uv = new double*[2];
	for (int i = 0; i < 2; ++i) {
		xy[i] = new double[points];
		uv[i] = new double[points];
	}
	for (int i = 0; i < points; i++) {
		xy[0][i] = data[2][i];
		xy[1][i] = data[3][i];
		uv[0][i] = data[4][i];
		uv[1][i] = data[5][i];
	}

	//theta and gamma values for camera pitch and yaw respectively, values calculated in Calibration class
	Calibration calib = Calibration::Calibration();
	calib.calibrate(m, n, cx, cy, cz, alpha_h, alpha_v, points, xy, uv);
	double theta = calib.theta;
	double gamma = calib.gamma;

	//desired pixel size of output
	int tv_width = getIntFromConsole("desired pixel width of output image", 400, 300, src.cols);
	int tv_height = getIntFromConsole("desired pixel height of output image", 800, 600, src.rows);

	//scaling factor of output
	double Sx = getDoubleFromConsole("desired x-coordinate scaling factor of output image", 5.0, 0.1, 100.0);
	double Sy = getDoubleFromConsole("desired y-coordinate scaling factor of output image", 5.0, 0.1, 100.0);

	//generate maps for u and v to insert into OpenCV remap function, values calculated in MapPair class
	MapPair maps = MapPair::MapPair();
	maps.generateMaps(m, n, cx, cy, cz, theta, gamma, alpha_h, alpha_v, tv_width, tv_height, Sx, Sy);
	Mat map_u = maps.u;
	Mat map_v = maps.v;

	//create empty map for destination image
	Mat dst;
	dst.create(tv_height, tv_width, CV_32FC1);

	//call remap function using src, dst, map_u, and map_v and set option for interpolation to bilinear
	remap(src, dst, map_u, map_v, CV_INTER_LINEAR, BORDER_CONSTANT, Scalar(0, 0, 0));

	//get region of interest from output image
	Mat ROI(dst, Rect(100, 200, 200, 400));
	Mat newdst;
	ROI.copyTo(newdst);

	//write resulting dst image into jpg file
	std::string filename = argv[1];
	filename = filename.substr(0, filename.length() - 4);
	std::ostringstream name;
	name << filename << "_" << ((int)Sx) << "x" << ((int)Sy) << "scale_" << tv_width << "x" << tv_height << ".jpg";
	imwrite(name.str(), dst);

	//display resulting image in GUI
	namedWindow("Display Image", CV_WINDOW_AUTOSIZE);
	imshow("Display Image", dst);
	waitKey(0);

	return 0;
}

