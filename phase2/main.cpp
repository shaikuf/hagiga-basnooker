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

int BALLS_FIND_DEBUG;

int main(int argc, char* argv[])
{
	// the resolution to use
	CvSize resolution = cvSize(1600, 1200);

	// get the chosen operating mode from the user
	cout<<"available modes:\n";
	cout<<"\t0: normal\n";
	cout<<"\t1: instance calibration\n";
	cout<<"\t2: learn edges\n";
	cout<<"\t3: template grabbing\n";
	cout<<"\t4: calibrate correlations\n";
	cout<<"\t5: calibrate holes\n";
	cout<<"\t6: general calibration\n";
	cout<<"\t-1: watch with corrections\n";

	int mode;
	do {
		cin>>mode;
	} while (mode<-1 || mode > 6);

	if(mode == 6) {
		// calibrate camera
		calibration(5, 3, 12, 5.8f, 5.8f, resolution, 0);
	} else if(mode == 1) {
		// calibrate viewpoint
		learnEdges(true, resolution, 0);
	} else if(mode == 0) {
		// main loop
		gameLoop(resolution, 0);
	} else if(mode == 3) {
		// grab templates
		grabTemplates(resolution, 0);
	} else if(mode == 2) {
		// learn edges
		learnEdges(false, resolution, 0);
	} else if(mode == 4) {
		// calibrate correlations
		calibrateCorrelationThds(resolution, 0);
	} else if (mode == 5){
		//clibrate holes
		calibrateHoles(resolution,0);
	}else if(mode == -1) {
		// watch with corrections
		watch(true, resolution, 0);
	}

	return 0;
}

void gameLoop(CvSize resolution, int device_id) {
	// init camera
	VideoCapture capture(device_id, resolution.width, resolution.height);
	IplImage *pre_image = capture.CreateCaptureImage();
	IplImage *image = capture.CreateCaptureImage();

	cvNamedWindow("Game", CV_WINDOW_AUTOSIZE);

	// load ball templates
	IplImage **ball_templates = ballTemplates();
	CvScalar* ball_inv_colors = ballInvColors();
	int* ball_counts = ballMaxCounts();
	double* ball_thd = ballCorrThds();
	bool* ball_inv_templ = ballInverseTempls();
	char* ball_tcp_prefix = ballTCPPrefixes();
	vector<CvPoint> ball_centers[8];

	// load projective transformation matrix
	CvMat* H;
	if(USE_BIRDS_EYE) {
		char filename[100];
		_snprintf_s(filename, 100, "H-%d.xml", device_id);
		H = (CvMat*)cvLoad(filename);
	}



	// initialization
	int i;
	bool find_balls = true;	// whether or not to find balls on the current
							// iteration of the main loop
	bool found_cue = false; // whether or not we found the cue on the current
							// iteration of the main loop
	bool checked_white = false; // whether or not we have checked if the white
								// has moved since there was movement on the
								// table
	__int64 freeze_time = -1;	// the time the movement in the image stopped

		// pyr-down versions of the image we check movement on.
		// image_pyr1 is a 2x downscale of the original,
		// and image_pyr2 is a 2x downscale of image_pyr1
	IplImage *image_pyr1 =
		cvCreateImage(cvSize(tableBordersBoundingRect().width/2,
		tableBordersBoundingRect().height/2),
		image->depth, image->nChannels);
	IplImage *image_pyr2 = 
		cvCreateImage(cvSize(image_pyr1->width/2, image_pyr1->height/2),
		image->depth, image->nChannels);

	TCPServer tcp_server;

	if(DRAW_BORDERS) {
		// for debug purposes
		printBorderAngles();
	}
	
	// main loop
	char c=0;
	while(c != 'q') {
		// check for messages from the client
		if(tcp_server.update())	// if we got a msg it means we need to
								// refetch the balls
			find_balls = true;

		// get and fix frame
		capture.waitFrame(pre_image); // capture frame
		if(USE_BIRDS_EYE) {
			cvWarpPerspective(pre_image, image,	H,
				CV_INTER_LINEAR | CV_WARP_INVERSE_MAP | CV_WARP_FILL_OUTLIERS);
		} else {
			cvCopy(pre_image, image);
		}

		// create downscaled versions for movement testing of the table area
		cvSetImageROI(image, tableBordersBoundingRect());
		cvPyrDown(image, image_pyr1);
		cvPyrDown(image_pyr1, image_pyr2);
		cvResetImageROI(image);

		// paint the holes black so they won't come up as a white ball or
		// white blobs of the cue
		paintHolesBlack(image);

		// find balls if needed
		if(FIND_BALLS) {
			if(find_balls) {
				// find the balls
				findBalls(image, ball_templates, ball_counts, ball_inv_templ, ball_thd,
					ball_centers, NUM_BALLS);

				// send to the client
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

			// mark the balls
			for(i=0; i<NUM_BALLS; i++) {
				for(unsigned int j=0; j < ball_centers[i].size(); j++)
					markCross(image, ball_centers[i][j], ball_inv_colors[i]);
			}
		}

		// find the cue
		if(FIND_CUE) {
			found_cue = false;
				// if we found the white -- look for the cue
			if(ball_centers[WHITE_INDEX].size() > 0) {
				double theta;

				// try to find the cue
				found_cue = findCueWithWhiteMarkers(image,
					ball_centers[WHITE_INDEX].front(), &theta, ball_centers,
					NUM_BALLS);

				if(found_cue) {
					// smooth and send to client
					theta = smoothTheta(theta);
					tcp_server.send_theta(theta);
				} else {
					tcp_server.send_raw("n\n");
				}
			}
		}

		// check for movements on the table
		if(AUTO_REFETCH) {
			// get the current time
			FILETIME now;
			GetSystemTimeAsFileTime(&now);
			__int64 cur_time = ((__int64)now.dwHighDateTime << 32) +
				now.dwLowDateTime;

			// check if there's movement
			bool moving = isMoving(image_pyr2);
			if(IS_MOVING_DEBUG) {
				cout<<"moving = "<<moving<<endl;
			}

			if(found_cue || moving) {
				// if there's movement, reset the freeze_time
				freeze_time = -1;
				checked_white = false;
			} else {
				// update the freeze time
				if(freeze_time == -1) {
					FILETIME now;
					GetSystemTimeAsFileTime(&now);
					freeze_time = cur_time;
				}

				// check if we're frozen for X time
				if(cur_time - freeze_time > IS_MOVING_WINDOW &&
					!checked_white) {

					// check if the white moved (actually we look for the pink
					// too because i'm lazy)
					vector<CvPoint> new_white[2];
					findBalls(image, ball_templates, ball_counts,
						ball_inv_templ, ball_thd, new_white,
						WHITE_INDEX+1);

					if((ball_centers[WHITE_INDEX].size() !=
						new_white[WHITE_INDEX].size()) ||
						((ball_centers[WHITE_INDEX].size() > 0) &&
						(dist(ball_centers[WHITE_INDEX][0],
						new_white[WHITE_INDEX][0]) >
						IS_MOVING_WHITE_DIST))) {
							// if it did move/appear/disappear

						if(IS_MOVING_DEBUG) {
							cout<<"white MOVED\n";
						}

						// send a request for "refetch" to the GUI
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
		
		// draw the current image
		if(DRAW_BORDERS) {
			drawBorders(image, 1);
		}
		cvShowImage("Game", image);
		c=cvWaitKey(50);
		if(c==' ')
			find_balls = true;
		else if(c=='d')
			BALLS_FIND_DEBUG = 1-BALLS_FIND_DEBUG;
	}

	// release stuff
	capture.stop();
	cvDestroyWindow("Game");

	cvReleaseImage(&pre_image);
	cvReleaseImage(&image);
	for(i=0; i<NUM_BALLS; i++)
		cvReleaseImage(&ball_templates[i]);
}