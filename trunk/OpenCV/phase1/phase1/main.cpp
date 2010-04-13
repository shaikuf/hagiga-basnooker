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
#include "tcp_server.h"

using namespace std;

int downscale_factor = 1;
bool save_images = false;

int main(int argc, char* argv[])
{
	CvSize resolution = cvSize(1600, 1200);
	//CvSize resolution = cvSize(800, 600);

	int mode = 0;
	/* available modes:
		0: normal
		1: instance calibration
		2: general calibration
		3: template grabbing
		4: learn borders
	*/

	if(mode == 2) {
		// calibrate camera
		calibration(4, 4, 12, 29.7, 21.0, resolution, 0);
	} else if(mode == 1) {
		// calibrate viewpoint
		birds_eye(4, 4, 29.7, 21.0, resolution, 0);
	} else if(mode == 0) {
		// main loop
		gameLoop(resolution);
	} else if(mode == 3) {
		// grab templates
		grab_templates(resolution, 0);
	} else if(mode == 4) {
		// learn borders
		learn_borders(resolution, 0);
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

	// load data from files and initialize
	int opt_count = 1;
	IplImage *templates[8];
	char *filenames[] = {"white-templ.jpg", "red-templ.jpg", "blue-templ.jpg",
		"yellow-templ.jpg", "green-templ.jpg", "pink-templ.jpg", "brown-templ.jpg",
		"black-templ.jpg"};
	CvScalar colors[8] = {cvScalar(0,0,0), cvScalar(255, 255, 0), cvScalar(0, 255, 255),
		cvScalar(255, 0, 0), cvScalar(255, 0, 255), cvScalar(52, 63, 0), cvScalar(255, 180, 105),
		cvScalar(255, 255, 255)};
	CvPoint2D32f ball_center[8];
	float ball_radius[8];

	int i;
	for(i=0; i<opt_count; i++) {
		templates[i] = cvLoadImage(filenames[i]);
		ball_center[i] = cvPoint2D32f(0, 0);
		ball_radius[i] = 1;
	}

	char filename[100];
	_snprintf_s(filename, 100, "H-%d.xml", device_id);
	CvMat* H = (CvMat*)cvLoad(filename);

	double cue_m;
	CvPoint cue_cm;

	bool find_balls = true;

	TCPServer tcp_server;
	
	// main loop
	
	char c=0;
	while(c != 'q') {
		/*if(c == 'c')
			find_balls = true;*/
		if(tcp_server.update())
			find_balls = true;

		// get and fix frame
		capture.waitFrame(pre_image); // capture frame
		cvWarpPerspective(pre_image, image,	H,
			CV_INTER_LINEAR | CV_WARP_INVERSE_MAP | CV_WARP_FILL_OUTLIERS);

		// find balls
		if(find_balls) {
			for(i=0; i<opt_count; i++)
				findBall(image, templates[i], &ball_center[i], &ball_radius[i], false);

			tcp_server.send_white_pos(ball_center[0].x, ball_center[0].y);
			find_balls = false;
		}

		// find cue

		// mark balls
		for(i=0; i<opt_count; i++) {
			markBall(image, ball_center[i], ball_radius[i], colors[i], false);
		}

		// mark cue
		markCue(image, ball_center[0], ball_radius[0], &cue_m, &cue_cm);
		
		if(cue_cm.x != 0 || cue_cm.y != 0) { // we found the cue
			float theta = line2theta(cue_m, cue_cm, ball_center[0]);
			
			cout<<"m="<<cue_m<<endl<<"cm.x="<<cue_cm.x<<" cm.y="<<cue_cm.y<<endl;
			cout<<"theta="<<theta<<endl;

			tcp_server.send_theta(theta);
		}

		cvShowImage("Game", image);
		c=cvWaitKey(100);
	}

	capture.stop();
	cvDestroyWindow("Game");

	cvReleaseImage(&image);
	for(i=0; i<opt_count; i++)
		cvReleaseImage(&templates[i]);
}