#ifndef _CUE_H
#define _CUE_H

#include <cv.h>
#include <vector>

using namespace std;

/* Convert the line from center of mass and slope to theta */
double line2theta(double cue_m, CvPoint cue_cm, CvPoint white_center);

/* Find the parameters of the cue using the white markers */
bool findCueWithWhiteMarkers(IplImage *src, CvPoint white_center, double *theta,
							 vector<CvPoint> *ball_centers, int ball_centers_num);

/* This keeps the last thetas and smooths the samples of it */
double smoothTheta(double new_theta);

#endif