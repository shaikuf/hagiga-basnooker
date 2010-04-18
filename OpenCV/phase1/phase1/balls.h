#ifndef _BALLS_H
#define _BALLS_H

#include <cv.h>

/* This finds the ball matching the given template on the image */
void findBall(IplImage *img, IplImage *templ, CvPoint2D32f *center,
			  float *radius);

/* This marks the ball on the image */
void markBall(IplImage *img, CvPoint2D32f center, float radius,
			  CvScalar color, bool draw_circle = true);

/* Fix an absolute position on the image, to a position relative to the table */
CvPoint2D32f fixPosition(CvPoint2D32f center);

#endif