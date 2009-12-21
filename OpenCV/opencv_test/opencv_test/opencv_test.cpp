// opencv_test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "VideoCapture.h"

int main( void )
{
	cvNamedWindow("Device 0");
	VideoCapture capt(0,1600,1200);
	IplImage * image = capt.CreateCaptureImage();
	char c=0;
	while(c!='q'){
		capt.waitFrame(image);
		cvShowImage("Device 0",image);
		c=cvWaitKey(1);
	}
	capt.stop();
	cvReleaseImage(&image);
	cvDestroyWindow("Device 0");
    return 0;
}
