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
		(int)balls_count	the number of balls in the array
	Returns:
		(bool)	whether or not we have found the cue */
bool findCueWithWhiteMarkers(IplImage *src, CvPoint white_center, double *theta,
							 vector<CvPoint> *ball_centers, int balls_count);

/* Return an averaging of the last given thetas over some time window
	Gets:
		(double)new_theta	the theta we found right now
	Returns:
		(double)	an averaging of the last given theta */
double smoothTheta(double new_theta);

/*	Tries to find the cue using both the white and black markers.
	Gets:
		(IplImage*)src			the source image to search on
		(CvPoint)white_center	the center of the white ball
		(double*)theta			an output variable for the angle of the cue
		(vector<CvPoint>*)ball_centers	an array of vectors containing the
										centers of the balls in the image
		(int)ball_count			the number of balls in the said array
	Returns:
		(bool) whether or not we have found the cue */
bool findCueWithAllMarkers(IplImage *src, CvPoint white_center, double *theta,
						   vector<CvPoint> *ball_centers, int ball_count);

/*	Finds the centers of both the white and black blobs in the image.
	Gets:
		(IplImage*)src		the image to search on
		(vector<CvPoint>*)ball_centers	an array of vectors containing the
										centers of the balls in the image
		(int)ball_count		the number of balls in the said array
		(bool)white			if true, look for the white blobs, and if false
							look for the black blobs
	Returns:
		(vector<CvPoint>)	the centers of the found blobs */
vector<CvPoint> findBlobs(IplImage *src, vector<CvPoint> *ball_centers,
							   int ball_count, bool white);

#endif