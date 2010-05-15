#include <iostream>
#include <cv.h>
#include <highgui.h>
#include "cue.h"
#include "misc.h"
#include <vector>
#include "linear.h"

using namespace std;

/* Convert the line from center of mass and slope to theta */
double line2theta(double cue_m, CvPoint cue_cm, CvPoint white_center) {
	double theta;

	if(isinf(cue_m)) { // perpendicular line
		if(cue_cm.y < white_center.y)
			theta = PI/2;
		else
			theta = 3*PI/2;

	} else { // non-perpendicular line
		cue_m = fabs(cue_m);
		if(cue_cm.x < white_center.x) {
			if(cue_cm.y < white_center.y) {
				// quarter 2
				cue_m *= -1;
				theta = PI + atan(cue_m);
			} else {
				// quarter 3
				theta = PI + atan(cue_m);
			}
		} else {
			if(cue_cm.y < white_center.y) {
				// quarter 1
				theta = atan(cue_m);
			} else {
				// quarter 4
				cue_m *= -1;
				theta = atan(cue_m) + 2*PI;
			}
		}
	}

	theta += PI;
	if(theta >= 2*PI)
		theta -= 2*PI;

	return theta;
}

bool findCueWithWhiteMarkers(IplImage *src, CvPoint white_center, double *theta) {
	static bool once = true;

	if(once && CUE_FIND_DEBUG) {
		cvNamedWindow("gray");
		cvNamedWindow("threshold");
		cvNamedWindow("morphed");
		once = false;
	}

	// create grayscale image
	IplImage* gray = createBlankCopy(src, 1);
	cvCvtColor(src, gray, CV_BGR2GRAY);

	if(CUE_FIND_DEBUG) {
		cvShowImage("gray", gray);
	}

	// TODO paint all balls black, because their centers are glowing
	// paint the white ball black so it wont be found
	cvCircle(gray, white_center,
		1, cvScalar(0), BALL_DIAMETER);

	// threshold to leave only white markers
	IplImage* thresh = createBlankCopy(gray);
	cvThreshold(gray, thresh, CUE_THRESH_VAL, 255, CV_THRESH_BINARY);

	if(CUE_FIND_DEBUG) {
		cvShowImage("threshold", thresh);
	}

	// morphological operations:
	IplImage *morph = cvCloneImage(thresh);

		// remove small objects
	cvErode(morph, morph, 0, CUE_OPENING_VAL);
	cvDilate(morph, morph, 0, CUE_OPENING_VAL);

		// merge large objects
	cvDilate(morph, morph, 0, CUE_CLOSING_VAL);
	cvErode(morph, morph, 0, CUE_CLOSING_VAL);

	if(CUE_FIND_DEBUG) {
		cvShowImage("morphed", morph);
	}

	// find the contours
	IplImage* temp = cvCloneImage(morph);

	CvMemStorage *mem_storage = cvCreateMemStorage(0);
	CvSeq* contours = NULL;

		// actually find the contours
	CvContourScanner scanner = cvStartFindContours(temp, mem_storage, \
		sizeof(CvContour), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	CvSeq* c;
	int numCont = 0;
	while( (c = cvFindNextContour( scanner )) != NULL ) {
		double len = cvContourPerimeter( c );

		if( len > CUE_BLOB_MAX_SIZE ) { // get rid of big blobs
			cvSubstituteContour( scanner, NULL );
		} else { // polynomial approximation
			CvSeq* c_new;

			c_new = cvApproxPoly(c, sizeof(CvContour), mem_storage,
				CV_POLY_APPROX_DP, 1, 0);

			cvSubstituteContour( scanner, c_new );
			numCont++;
		}
	}
	contours = cvEndFindContours( &scanner );

		// find the contour centers
	int i;
	vector<CvPoint2D32f> centers;
	CvMoments moments;
	double M00, M01, M10;

	for(i=0, c=contours; c != NULL; c = c->h_next, i++) {
		cvContourMoments(c, &moments);

		M00 = cvGetSpatialMoment(&moments,0,0);
		M10 = cvGetSpatialMoment(&moments,1,0);
		M01 = cvGetSpatialMoment(&moments,0,1);
		centers.push_back(cvPoint2D32f((M10/M00),(M01/M00)));
	}

		// filter those outside the borders by far
	centers = filterPointsOnTable(centers, CUE_BLOB_MAX_DIST_FROM_TABLE);

	// find the linear regression
	double cue_n;
	double cue_m;
	CvPoint cue_cm;

	vector<CvPoint> real_centers = findPointsOnLine(centers, CUE_MIN_COEFF, &cue_m,
		&cue_n, &cue_cm);

	// perhaps filter the line
	bool found_cue = true;
	if(cue_cm.x == -1) { // did we even find one?
		found_cue = false;
	} else { // check if the line is around the white ball
		double d;
		if((d = distFromLine(white_center.x, white_center.y, cue_n, cue_m)) > CUE_MAX_DIST_FROM_WHITE) {
			// ignore this cue
			found_cue = false;
		}
		if(CUE_FIND_DEBUG) {
			cout<<d<<endl;
		}
	}

	// paint debug image
	if(CUE_FIND_DEBUG) {
		for(i=0; i < (int)real_centers.size(); i++) {
			if(found_cue) {
				cvCircle(src, cvPoint(real_centers[i].x, real_centers[i].y), 1,
					cvScalar(0, 255, 0), 3);
			} else {
				cvCircle(src, cvPoint(real_centers[i].x, real_centers[i].y), 1,
					cvScalar(0, 0, 255), 3);
			}
		}
	}

	// calculate the angle
	if(found_cue) {
		*theta = line2theta(cue_m, cue_cm, white_center);
	}

	// release stuff
	cvReleaseMemStorage(&mem_storage);

	cvReleaseImage(&gray);
	cvReleaseImage(&thresh);
	cvReleaseImage(&morph);
	cvReleaseImage(&temp);

	return found_cue;
}

