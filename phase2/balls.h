#ifndef _BALLS_H
#define _BALLS_H

#include <cv.h>
#include <vector>

using namespace std;

/* This finds the ball matching the given template on the image */
vector<CvPoint> findBall(IplImage *img, IplImage *templ, int max_count,
						 bool invert = false);

/* This marks the ball on the image */
void markBall(IplImage *img, CvPoint center, CvScalar color);

/* Fix an absolute position on the image, to a position relative to the table */
CvPoint2D32f fixPosition(CvPoint2D32f center);

#endif