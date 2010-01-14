#ifndef _BALLS_H
#define _BALLS_H

#include <cv.h>

/* Find circles using Hough Transform, and draw them on the returned image,
overlayed on the original image. */
IplImage *findCircles(IplImage *src, int canny, int thresh, int min_dist,
					  int min_radius, int max_radius);

/* Finds the center and radius of the maximal circle containing the point
given, and not containing any non-zero point in the given image. */
void findMaxContainedCircle(IplImage *bw, CvPoint inside,
							CvPoint2D32f *f_center, float *f_radius,
							int thresh);

/* This tries to find the center and radius of the ball which contains
the given point, in the image. */
void findBallAround(IplImage *src, CvPoint inside, CvPoint2D32f *center,
					float *radius, bool debug = true);

/* This finds the ball matching the given template on the image */
void findBall(IplImage *img, IplImage *templ, CvPoint2D32f *center,
			  float *radius, bool debug = true);

/* This uses findBall() and marks the results on the image */
void markBall(IplImage *img, IplImage *templ, bool circle = true);

#endif