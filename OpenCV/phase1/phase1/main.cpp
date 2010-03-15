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
		calibration(5, 3, 12, 4.95f, resolution, 0);
	} else if(mode == 1) {
		// calibrate viewpoint
		birds_eye(5, 3, resolution, 0);
	} else if(mode == 0) {
		// main loop
		gameLoop(resolution);
	} else if(mode == 3) {
		// grab templates
		grab_templates(resolution, 0);
	}

	return 0;
}

/* The main loop of the program */
void gameLoop(CvSize resolution) {
	// init camera
	int device_id = 0;

	VideoCapture capture(device_id, resolution.width, resolution.height);
	IplImage *pre_image = capture.CreateCaptureImage();
	IplImage *image = capture.CreateCaptureImage();

	cvNamedWindow("Game", CV_WINDOW_AUTOSIZE);

	// load data from files
	int opt_count = 1;
	IplImage *templates[2];
	char *filenames[] = {"white-templ.jpg", "red-templ.jpg"};

	int i;
	for(i=0; i<opt_count; i++)
		templates[i] = cvLoadImage(filenames[i]);

	char filename[100];
	_snprintf_s(filename, 100, "Intrinsics-%d.xml", device_id);
	CvMat* intrinsic = (CvMat*)cvLoad(filename);
	_snprintf_s(filename, 100, "Distortion-%d.xml", device_id);
	CvMat* distortion = (CvMat*)cvLoad(filename);
	_snprintf_s(filename, 100, "H-%d.xml", device_id);
	CvMat* H = (CvMat*)cvLoad(filename);

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
			findBall(image, templates[0], &white_center, &white_radius, true);
			find_balls = false;
		}

		markBall(image, white_center, white_radius, false);

		cvShowImage("Game", image);
		c=cvWaitKey(1);
	}

	capture.stop();
	cvDestroyWindow("Game");

	cvReleaseImage(&image);
	for(i=0; i<opt_count; i++)
		cvReleaseImage(&templates[i]);
}