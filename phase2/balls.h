#ifndef _BALLS_H
#define _BALLS_H

#include <cv.h>
#include <vector>

#define DEBUG_FIX_POS 0

/* This finds the ball matching the given template on the image */
void findBall(IplImage *img, IplImage *templ, CvPoint2D32f *center,
			  float *radius);

#define BALL_CORR_THD 0.85

/* This finds the balls matching the given template on the image */
vector<CvPoint> findBall(IplImage *img, IplImage *templ, int max_count,
						 bool invert = false);

/* Fix an absolute position on the image, to a position relative to the table */
CvPoint2D32f fixPosition(CvPoint center);

#endif