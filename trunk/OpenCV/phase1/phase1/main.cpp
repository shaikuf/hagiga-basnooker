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
#include "VideoCapture.h"

using namespace std;

int downscale_factor = 1;
bool save_images = false;

int main(int argc, char* argv[])
{
	//CvSize resolution = cvSize(1600, 1200);
	CvSize resolution = cvSize(800, 600);

	int mode = 3;
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