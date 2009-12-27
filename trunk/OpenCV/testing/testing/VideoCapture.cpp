#include "VideoCapture.h"

VideoCapture::VideoCapture(int index, int w=1600, int h=1200){
	_device=index;
	_width = w;
	_height = h;

	_VI.setupDevice(_device, _width, _height);
}

bool VideoCapture::restart(){
	return _VI.setupDevice(_device, _width, _height);
}

IplImage *VideoCapture::CreateCaptureImage(){
	CvSize size;
	size.width=_VI.getWidth(_device);
	size.height=_VI.getHeight(_device);
	return cvCreateImage(size,IPL_DEPTH_8U,3);
}

void VideoCapture::stop()
{
	_VI.stopDevice(_device);
}

VideoCapture::~VideoCapture(){
	_VI.stopDevice(_device);
}
bool VideoCapture::getFrame(IplImage *frame){
	if(_VI.isFrameNew(_device)){
		return _VI.getPixels(_device, (unsigned char *)frame->imageData, false, true);
	}
	return false;
}
bool VideoCapture::waitFrame(IplImage *frame){
	while(!_VI.isFrameNew(_device)){}
	return _VI.getPixels(_device, (unsigned char *)frame->imageData, false, true);
}