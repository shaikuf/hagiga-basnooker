#ifndef _VIDEOCAPTURE_H
#define _VIDEOCAPTURE_H

#include <cxcore.h>
#include <videoInput.h>

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
