#include "VideoCapture.h"
#include <highgui.h>

void loopCam()
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
}
