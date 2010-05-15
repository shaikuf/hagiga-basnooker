#ifndef _CUE_H
#define _CUE_H

#include <cv.h>
#include <vector>

using namespace std;

#define CUE_FIND_DEBUG 1
#define CUE_MIN_COEFF 0.99
#define CUE_MAX_DIST_FROM_WHITE 15.0
#define CUE_THRESH_VAL 220
#define CUE_OPENING_VAL 2
#define CUE_CLOSING_VAL 4
#define CUE_BLOB_MAX_SIZE 100
#define CUE_BLOB_MAX_DIST_FROM_TABLE 50

/* Convert the line from center of mass and slope to theta */
double line2theta(double cue_m, CvPoint cue_cm, CvPoint white_center);

/* Find the parameters of the cue using the white markers */
bool findCueWithWhiteMarkers(IplImage *src, CvPoint white_center, double *theta,
							 vector<CvPoint> *ball_centers, int ball_centers_num);


#endif