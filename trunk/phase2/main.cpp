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

int main(int argc, char* argv[])
{
	CvSize resolution = cvSize(1600, 1200);
	//CvSize resolution = cvSize(800, 600);

	cout<<"available modes:\n";
	cout<<"\t0: normal\n\t1: instance calibration\n\t2: general calibration\n";
	cout<<"\t3: template grabbing\n\t4: learn borders\n\t5: learn edges\n";
	cout<<"\t-1: watch with corrections\n";

	int mode = -1;
	do {
		cin>>mode;
	} while (mode<-1 || mode > 5);

	if(mode == 2) {
		// calibrate camera
		calibration(5, 3, 12, 5.8f, 5.8f, resolution, 0);
	} else if(mode == 1) {
		// calibrate viewpoint
		birds_eye(5, 3, 5.8f, 5.8f, resolution, 0);
	} else if(mode == 0) {
		// main loop
		gameLoop(resolution, 0);
	} else if(mode == 3) {
		// grab templates
		grab_templates(resolution, 0);
	} else if(mode == 4) {
		// learn borders
		learn_borders(resolution, 0);
	} else if(mode == 5) {
		// learn edges
		learn_edges(resolution, 0);
	} else if(mode == -1) {
		// watch with corrections
		watch(resolution, true, 0);
	}

	return 0;
}

/* The main loop of the program */
void gameLoop(CvSize resolution, int device_id) {
	// init camera
	VideoCapture capture(device_id, resolution.width, resolution.height);
	IplImage *pre_image = capture.CreateCaptureImage();
	IplImage *image = capture.CreateCaptureImage();

	cvNamedWindow("Game", CV_WINDOW_AUTOSIZE);

	// load ball templates
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
	for(i=0; i<NUM_BALLS; i++) {
		templates[i] = cvLoadImage(filenames[i]);
		ball_center[i] = cvPoint2D32f(0, 0);
		ball_radius[i] = 1;
	}

	// load projective transformation matrix
	CvMat* H;
	if(USE_BIRDS_EYE) {
		char filename[100];
		_snprintf_s(filename, 100, "H-%d.xml", device_id);
		H = (CvMat*)cvLoad(filename);
	}

	// initialization
	double cue_m;
	CvPoint cue_cm;

	bool find_balls = true;

	TCPServer tcp_server;
	
	// main loop
	char c=0;
	while(c != 'q') {
		// check for messages from the client
		if(tcp_server.update())
			find_balls = true;

		// get and fix frame
		capture.waitFrame(pre_image); // capture frame
		if(USE_BIRDS_EYE) {
			cvWarpPerspective(pre_image, image,	H,
				CV_INTER_LINEAR | CV_WARP_INVERSE_MAP | CV_WARP_FILL_OUTLIERS);
		} else {
			cvCopy(pre_image, image);
		}

		// find balls
		if(find_balls) {
			for(i=0; i<NUM_BALLS; i++)
				findBall(image, templates[i], &ball_center[i], &ball_radius[i]);

			find_balls = false;

			// send white ball to client
			CvPoint2D32f normed_pos = fixPosition(ball_center[0]);
			tcp_server.send_white_pos(normed_pos.x, normed_pos.y);
			cout<<"Sending: ("<<normed_pos.x<<", "<<normed_pos.y<<")\n";
		}

		// mark balls
		for(i=0; i<NUM_BALLS; i++) {
			markBall(image, ball_center[i], ball_radius[i], colors[i], false);
		}

		// mark cue
		findAndMarkCue(image, ball_center[0], ball_radius[0], &cue_m, &cue_cm);
		
		if(cue_cm.x != 0 || cue_cm.y != 0) { // we found the cue
			double theta = line2theta(cue_m, cue_cm, ball_center[0]);
			
			// send angle to client
			tcp_server.send_theta(theta);
		}

		cvShowImage("Game", image);
		c=cvWaitKey(100);
	}

	capture.stop();
	cvDestroyWindow("Game");

	cvReleaseImage(&pre_image);
	cvReleaseImage(&image);
	for(i=0; i<NUM_BALLS; i++)
		cvReleaseImage(&templates[i]);
}