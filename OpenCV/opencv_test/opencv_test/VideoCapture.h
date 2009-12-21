#ifndef VCAPTURE
#define VCAPTURE 77
#include <cv.h>
#include <cxcore.h>
#include <highgui.h>
#include <cvaux.h>
#include <videoInput.h>
#include <string>
#include <iostream>
#include <fstream>
using namespace std;

class VideoCapture {
	videoInput _VI;

	int _device;
	int _height;
	int _width;

public:
	VideoCapture(int index, int w, int h);
	void stop();
	bool restart();

	IplImage *CreateCaptureImage();

	~VideoCapture();
	bool getFrame(IplImage *frame);
	bool waitFrame(IplImage *frame);
};

#endif