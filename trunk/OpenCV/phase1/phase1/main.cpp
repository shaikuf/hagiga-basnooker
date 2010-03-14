#include <iostream>
#include <windows.h>
#include <highgui.h>
#include <cv.h>
#include <time.h>
#include <limits.h>
#include "main.h"
#include "misc.h"
#include "cue.h"
#include "balls.h"
#include "calibration.h"
#include "camera_interface.h"
#include "VideoCapture.h"

using namespace std;

int downscale_factor = 1;
bool save_images = false;

int main(int argc, char* argv[])
{
	//CvSize resolution = cvSize(1600, 1200);
	CvSize resolution = cvSize(800, 600);

	int mode = 0;
	/* available modes:
		0: normal
		1: instance calibration
		2: general calibration
		3: template grabbing
	*/

	if(mode == 2) {
		// calibrate camera
		calibration(5, 3, 12, 4.95f, resolution);
	} else if(mode == 1) {
		// calibrate viewpoint
		birds_eye(5, 3, resolution);
	} else if(mode == 0) {
		// main loop
		gameLoop(resolution);
	} else if(mode == 3) {
		// grab templates
		grab_templates(resolution);
	}

	return 0;
}

/* Shows an image and forwared clicks on it to the given function until ESC is
met */
void genericMouseWrapper(IplImage *img, void (*func)(int, int, int, int, void *)) {
	cvNamedWindow("MouseWrapper", CV_WINDOW_AUTOSIZE);
	cvShowImageWrapper("MouseWrapper", img);

	cvSetMouseCallback("MouseWrapper", func, img);
	while(1) {
		if(cvWaitKey(15) == 27)
			break;
	}
	
	cvDestroyWindow("MouseWrapper");
}

/* Mouse callback wrapper that prints the color of the clicked point. param
is the img to work on */
void findColorAroundMouse(int event, int x, int y, int flags, void *param) {
	if(event != CV_EVENT_LBUTTONUP)
		return;

	IplImage *img = (IplImage *)param;

	CvScalar color = cvGet2D(img, y, x);
	for(int i=0; i<img->nChannels; i++) {
		cout<<color.val[i]<<" ";
	}
	cout<<endl;

	IplImage *temp = cvCloneImage(img);
	cvCircle(temp, cvPoint(x, y), 1, cvScalar(0xff,0,0), 2);
	cvShowImageWrapper("MouseWrapper", temp);
	cvReleaseImage(&temp);
}

/* Mouse callback wrapper that prints the position of the clicked point.
param is the img to work on */
void findPosAroundMouse(int event, int x, int y, int flags, void *param) {
	if(event != CV_EVENT_LBUTTONUP)
		return;

	IplImage *img = (IplImage *)param;

	cout<<"(x,y) = ("<<x<<", "<<y<<")\n";

	IplImage *temp = cvCloneImage(img);
	cvCircle(temp, cvPoint(x, y), 1, cvScalar(0xff,0,0), 2);
	cvShowImageWrapper("MouseWrapper", temp);
	cvReleaseImage(&temp);
}

/* Mouse callback wrapper for findBallAround. param is the img to work on */
void findBallAroundMouse(int event, int x, int y, int flags, void *param) {
	if(event != CV_EVENT_LBUTTONUP)
		return;

	IplImage *img = (IplImage *)param;

	x = (int)(((double)x)/1.5625);
	y = (int)(((double)y)/1.5625);

	// find the ball parameters
	CvPoint2D32f center;
	float radius;
	findBallAround(img, cvPoint(x,y), &center, &radius);

	// print an overlay image of the found circle
	IplImage *overlay = createBlankCopy(img);
	IplImage *overlay_drawing = createBlankCopy(overlay, 1);
	IplImage *overlay_blank = createBlankCopy(overlay, 1);

	cvSet(overlay_blank, cvScalar(0));

	cvCircle(overlay_drawing, cvPoint(cvRound(center.x), cvRound(center.y)),
		cvRound(radius), cvScalar(0xff), 2);
	int line_len = 5;
	cvLine(overlay_drawing, cvPoint(cvRound(center.x)-line_len,
		cvRound(center.y)), cvPoint(cvRound(center.x)+line_len,
		cvRound(center.y)), cvScalar(0xff), 2);
	cvLine(overlay_drawing, cvPoint(cvRound(center.x),
		cvRound(center.y)-line_len), cvPoint(cvRound(center.x),
		cvRound(center.y)+line_len), cvScalar(0xff), 2);

	cvMerge(overlay_blank, overlay_blank, overlay_drawing, 0, overlay);

	IplImage *temp = cvCloneImage(img);
	cvAddWeighted(img, 0.5, overlay, 1, 0, temp);

	cvReleaseImage(&overlay);
	cvReleaseImage(&overlay_drawing);
	cvReleaseImage(&overlay_blank);

	// DEBUG -- print a cross where the mouse is
	/*IplImage *temp = cvCloneImage(img);

	cvLine(temp, cvPoint(x-5, y),
		cvPoint(x+5, y), cvScalar(0xff), 2);
	cvLine(temp, cvPoint(x, y-5),
		cvPoint(x, y+5), cvScalar(0xff), 2);*/

	cvShowImageWrapper("MouseWrapper", temp);

	cvReleaseImage(&temp);
}

/* The main loop of the program */
void gameLoop(CvSize resolution) {
	// init camera
	VideoCapture capture(0, resolution.width, resolution.height);
	IplImage *pre_image = capture.CreateCaptureImage();
	IplImage *image = capture.CreateCaptureImage();

	cvNamedWindow("Game", CV_WINDOW_AUTOSIZE);

	// load data from files
	IplImage *white_templ = cvLoadImage("C:\\Projecton\\Test\\Testing\\"
		"WhiteBall.jpg");

	CvMat* intrinsic = (CvMat*)cvLoad("Intrinsics.xml");
	CvMat* distortion = (CvMat*)cvLoad("Distortion.xml");
	CvMat* H = (CvMat*)cvLoad("H.xml");

	IplImage* mapx = cvCreateImage( cvGetSize(image), IPL_DEPTH_32F, 1 );
	IplImage* mapy = cvCreateImage( cvGetSize(image), IPL_DEPTH_32F, 1 );
	cvInitUndistortMap(
		intrinsic,
		distortion,
		mapx,
		mapy
	);

	// initializations
	bool find_balls = true;
	CvPoint2D32f white_center;
	float white_radius;


	// main loop
	// markBall(image, white_templ, &white_center, &white_radius, false);

	char c=0;
	while(c != 'q') {
		capture.waitFrame(pre_image); // capture frame
		cvWarpPerspective(pre_image, image,	H,
			CV_INTER_LINEAR | CV_WARP_INVERSE_MAP | CV_WARP_FILL_OUTLIERS);

		if(find_balls) {
			findBall(image, white_templ, &white_center, &white_radius, true);
			find_balls = false;
		}

		markBall(image, white_center, white_radius, false);

		cvShowImage("Game", image);
		c=cvWaitKey(1);
	}

	capture.stop();
	cvDestroyWindow("Game");

	cvReleaseImage(&image);
	cvReleaseImage(&white_templ);
}