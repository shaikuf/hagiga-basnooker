#ifndef _MISC_H
#define _MISC_H

#include <cv.h>
#include <limits>
#include <vector>
#include "params.h"

using namespace std;

#define PI 3.14159265

/* Create a new IplImage, the same size of src. If channels is -1, it's also
the same number of channels. The same for depth. */
IplImage *createBlankCopy(IplImage *src, int channels = -1, int depth = -1);

/* This returns a sequence of CvPoints specifying the contour of the
board borders */
CvSeq *tableBorders();

/* Get the bounding rect of the table borders */
CvRect tableBordersBoundingRect();

/* This draws the borders of the table */
void drawBorders(IplImage *dst, int width);

/* This filters points which are outside tableBorders() */
vector<CvPoint2D32f> filterPointsOnTable(const vector<CvPoint2D32f> &centers,
											  double max_dist);
bool isPointOnTable(CvPoint2D32f p, double max_dist);
bool isPointOnTable(CvPoint p, double max_dist);

/* This marks a cross on the image */
void markCross(IplImage *img, CvPoint center, CvScalar color);

/* this prints the angles of the edges calibration */
void printBorderAngles();

/* paints the holes of the table black */
void paintHolesBlack(IplImage *img);

/* this checks if a variable (double) equals infinity */
template<typename T>
inline bool isinf(T value) {
	return std::numeric_limits<T>::has_infinity &&
		value == std::numeric_limits<T>::infinity();
}

/* check if an image is changing over time */
bool isMoving(IplImage *img);

/* l2 distance between points */
double dist(CvPoint p1, CvPoint p2);

#endif