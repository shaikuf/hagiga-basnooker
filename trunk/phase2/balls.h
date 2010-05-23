#ifndef _BALLS_H
#define _BALLS_H

#include <cv.h>
#include <vector>

using namespace std;

#define BALL_CORR_THD 0.75

/* Fix an absolute position on the image, to a position relative to the table */
CvPoint2D32f fixPosition(CvPoint center);

/* This finds all the balls at once */
void findBalls(IplImage *img, IplImage *ball_templates[],
						 int ball_counts[], bool ball_inv_templ[],
						 double ball_thds[], vector<CvPoint> ball_centers[],
						 int n_balls);

#endif