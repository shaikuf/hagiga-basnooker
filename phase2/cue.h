#ifndef _CUE_H
#define _CUE_H

#include <cv.h>
#include <vector>

using namespace std;

/*	Converts the line from center of mass and slope to theta (which is modified
	for the GUI)
	Gets:
		(double)cue_m			the slope of the cue
		(CvPoint)cue_cm			the center of mass of the cue
		(CvPoint)white_center	the center of the white ball
	Returns:
		(double)	the angle of the cue (or something similar because it was
					modified for the GUI usage :|)*/
double line2theta(double cue_m, CvPoint cue_cm, CvPoint white_center);

/*	Finds the parameters of the cue using the white markers
	Gets:
		(IplImage*)src		the image to search the cue for
		(CvPoint)white_center	the center of the white ball
		(double*)theta		a output variable for the found cue angle
		(vector<CvPoint>*)ball_centers	a vector of the ball centers
	Returns:
		(bool)	whether or not we found the cue */
bool findCueWithWhiteMarkers(IplImage *src, CvPoint white_center, double *theta,
							 vector<CvPoint> *ball_centers);

/* Return an averaging of the last given thetas over some time window
	Gets:
		(double)new_theta	the theta we found right now
	Returns:
		(double)	an averaging of the last given theta */
double smoothTheta(double new_theta);

#endif