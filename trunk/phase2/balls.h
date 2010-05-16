#ifndef _BALLS_H
#define _BALLS_H

#include <cv.h>
#include <vector>

using namespace std;

#define BALL_CORR_THD 0.85

/* This finds the balls matching the given template on the image */
vector<CvPoint> findBall(IplImage *img, IplImage *templ, int max_count,
						 bool invert = false);

/* Fix an absolute position on the image, to a position relative to the table */
CvPoint2D32f fixPosition(CvPoint center);

#endif