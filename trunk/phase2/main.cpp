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

	cout<<"available modes:\n";
	cout<<"\t0: normal\n\t1: instance calibration\n\t2: general calibration\n";
	cout<<"\t3: template grabbing\n\t4: learn edges\n";
	cout<<"\t-1: watch with corrections\n";

	int mode;
	do {
		cin>>mode;
	} while (mode<-1 || mode > 5);

	if(mode == 2) {
		// calibrate camera
		calibration(5, 3, 12, 5.8f, 5.8f, resolution, 0);
	} else if(mode == 1) {
		// calibrate viewpoint
		learn_edges(true, resolution, 0);
		//birds_eye(12, 8, 5.8f, 5.8f, resolution, 0);
	} else if(mode == 0) {
		// main loop
		gameLoop(resolution, 0);
	} else if(mode == 3) {
		// grab templates
		grab_templates(resolution, 0);
	} else if(mode == 4) {
		// learn edges
		learn_edges(false, resolution, 0);
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
	IplImage *ball_templates[8];
	char *ball_filenames[8] = {"white-templ.jpg", "pink-templ.jpg",
		"red-templ.jpg", "yellow-templ.jpg", "green-templ.jpg",
		"brown-templ.jpg", "blue-templ.jpg",  "black-templ.jpg"};
	CvScalar ball_inv_colors[8] = {cvScalar(0,0,0), cvScalar(52, 63, 0),
		cvScalar(255, 255, 0), cvScalar(255, 0, 0), cvScalar(255, 0, 255),
		cvScalar(255, 180, 105), cvScalar(0, 255, 255), 
		cvScalar(255, 255, 255)};
	int ball_counts[8] = {1, 1, 15, 1, 1, 1, 1, 1};
	double ball_thd[8] = {0.9, 0.9, 0.8, 0.9, 0.75, 0.7, 0.85, 0.17};
	bool ball_inv_templ[8] = {false, false, false, false, false, false, false,
		true};
	char ball_tcp_prefix[8] = {'w', 'p', 'r', 'y', 'g', 'o', 'l', 'b'};
	vector<CvPoint> ball_centers[8];

	int i;
	for(i=0; i<NUM_BALLS; i++) {
		ball_templates[i] = cvLoadImage(ball_filenames[i]);
	}

	// load projective transformation matrix
	CvMat* H;
	if(USE_BIRDS_EYE) {
		char filename[100];
		_snprintf_s(filename, 100, "H-%d.xml", device_id);
		H = (CvMat*)cvLoad(filename);
	}

	// initialization
	bool find_balls = true;
	bool found_cue = false;
	bool checked_white = false;
	__int64 freeze_time = -1;
	IplImage *image_pyr1 = 
		cvCreateImage(cvSize(tableBordersBoundingRect().width/2,
		tableBordersBoundingRect().height/2),
		image->depth, image->nChannels);
	IplImage *image_pyr2 = 
		cvCreateImage(cvSize(image_pyr1->width/2, image_pyr1->height/2),
		image->depth, image->nChannels);

	TCPServer tcp_server;

	printBorderAngles();
	
	// main loop
	char c=0;
	int theta_sending_i = 0;
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

		cvSetImageROI(image, tableBordersBoundingRect());
		cvPyrDown(image, image_pyr1);
		cvPyrDown(image_pyr1, image_pyr2);
		cvResetImageROI(image);

		paintHolesBlack(image);

		// find balls if needed
		if(FIND_BALLS) {
			if(find_balls) {
				findBalls(image, ball_templates, ball_counts, ball_inv_templ, ball_thd,
					ball_centers, NUM_BALLS);

				// send to client
				tcp_server.send_raw("start\n");
				CvPoint2D32f normed_pos;
				for(i=0; i<NUM_BALLS; i++) {
					for(unsigned int j=0; j<ball_centers[i].size(); j++) {
						normed_pos = fixPosition(ball_centers[i][j]);
						tcp_server.send_ball_pos(normed_pos.x, normed_pos.y, ball_tcp_prefix[i]);
						cout<<"Sending: "<<ball_tcp_prefix[i]<<normed_pos.x<<","<<
							normed_pos.y<<endl;
					}
				}
				tcp_server.send_raw("end\n");

				find_balls = false;
			}

			// mark balls
			for(i=0; i<NUM_BALLS; i++) {
				for(unsigned int j=0; j < ball_centers[i].size(); j++)
					markCross(image, ball_centers[i][j], ball_inv_colors[i]);
			}
		}

		// find the cue
		if(FIND_CUE) {
			found_cue = false;
			if(ball_centers[0].size() > 0) { // we found the white
				double theta;

				found_cue = findCueWithWhiteMarkers(image, ball_centers[0].front(), &theta,
					ball_centers, NUM_BALLS);

				if(found_cue)
					theta = smoothTheta(theta);

				theta_sending_i = (theta_sending_i+1)%CUE_THETA_DOWNSAMPLE;
				if(found_cue && theta_sending_i == 0) { // we found the cue
					// send angle to client
					tcp_server.send_theta(theta);
				}
			}
		}

		// check for movements
		if(AUTO_REFETCH) {
			FILETIME now;
			GetSystemTimeAsFileTime(&now);
			__int64 cur_time = ((__int64)now.dwHighDateTime << 32) +
				now.dwLowDateTime;

			// update freeze time
			bool moving = isMoving(image_pyr2);
			if(IS_MOVING_DEBUG) {
				cout<<"moving = "<<moving<<endl;
			}

			if(found_cue || moving) {
				freeze_time = -1;
				checked_white = false;
			} else {
				if(freeze_time == -1) {
					FILETIME now;
					GetSystemTimeAsFileTime(&now);
					freeze_time = cur_time;
				}

				// check if we're frozen for X time
				if(cur_time - freeze_time > IS_MOVING_WINDOW && !checked_white) {
					// check if the white moved
					vector<CvPoint> new_white;
					findBalls(image, ball_templates, ball_counts, ball_inv_templ, ball_thd,
						&new_white, 1);

					if((ball_centers[0].size() != new_white.size()) ||
						((ball_centers[0].size() > 0) &&
						(dist(ball_centers[0][0], new_white[0]) >
						IS_MOVING_WHITE_DIST))) {

						if(IS_MOVING_DEBUG) {
							cout<<"white MOVED\n";
						}

						tcp_server.send_refetch();
					} else {
						if(IS_MOVING_DEBUG) {
							cout<<"white did not move\n";
						}
					}

					checked_white = true;
				}
			}
		}
		
		// draw the image
		if(DRAW_BORDERS) {
			drawBorders(image, 1);
		}
		cvShowImage("Game", image);
		c=cvWaitKey(100);
	}

	// release stuff
	capture.stop();
	cvDestroyWindow("Game");

	cvReleaseImage(&pre_image);
	cvReleaseImage(&image);
	for(i=0; i<NUM_BALLS; i++)
		cvReleaseImage(&ball_templates[i]);
}