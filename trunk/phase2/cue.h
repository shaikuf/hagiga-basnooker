#ifndef _CUE_H
#define _CUE_H

#include <cv.h>
#include <vector>

using namespace std;

#define CUE_PYRDOWN_WIDTH 1600
#define CUE_PYRDOWN_HEIGHT 1200

#define CUE_FIND_DEBUG 1

/* Convert the line from center of mass and slope to theta */
double line2theta(double cue_m, CvPoint cue_cm, CvPoint white_center);

/* Find the angle of the cue using the white markers */
void findCueWithWhiteMarkers(IplImage *src, double *cue_m, CvPoint *cue_cm,
							 CvPoint white_center);


#endif