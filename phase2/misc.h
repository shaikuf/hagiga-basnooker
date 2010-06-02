#ifndef _MISC_H
#define _MISC_H

#include <cv.h>
#include <limits>
#include <vector>
#include "params.h"

using namespace std;

#define PI 3.14159265

/*	Create a new blank IplImage, the same size of src.
	Gets:
		(IplImage*)src	the image to create a copy of
		(int)channels	the number of channels in the copy. if it's -1 then
						the same as src
		(int)depth		the depth of the image channels. if it's -1 then the
						same as src
	Returns:
		(IplImage*)		the blank copy */
IplImage *createBlankCopy(IplImage *src, int channels = -1, int depth = -1);

/*	Returns a sequence of CvPoints specifying the contour of the projection
	borders.
	Returns:
		(CvSeq*)	the said sequence*/
CvSeq *tableBorders();

/*	Get the bounding rect of the table borders.
	Returns:
		(CvRect)	the said rect*/
CvRect tableBordersBoundingRect(int offset = BOUNDING_RECT_OFFSET);

/*	Draw the borders of the table on an image.
	Gets:
		(IplImage*)dst	the image to draw on
		(int)width		the width of the border lines to draw */
void drawBorders(IplImage *dst, int width);

/*	Filters points which are outside tableBorders().
	Gets:
		(const vector<CvPoint2D32f>&)centers	the input points vector
		(double)max_dist	the maximal distance of a point from the table
							(for more than that we filter it)
	Returns:
		(vector<CvPoint2D32f>)	the points which passed the filtering */
vector<CvPoint2D32f> filterPointsOnTable(const vector<CvPoint2D32f> &centers,
											  double max_dist);

/*	Returns whether or not a point is on the table.
	Gets:
		(CvPoint2D32f)p		the point to check
		(double)max_dist	the maximal distance we tolerate */
bool isPointOnTable(CvPoint2D32f p, double max_dist);
/* The same as the previous, only for CvPoint */
bool isPointOnTable(CvPoint p, double max_dist);

/*	Mark a cross on the image.
	Gets:
		(Iplimage*)img	the image to mark on
		(CvPoint)center	the center of the cross
		(CvScalar)color	the color of the cross */
void markCross(IplImage *img, CvPoint center, CvScalar color);

/*	Prints the angles of the table borders. */
void printBorderAngles();

/* Paints the holes of the table black on an image.
	Gets:
		(IplImage*)img	the image to draw on */
void paintHolesBlack(IplImage *img);

/* Check if a variable (double) equals infinity */
template<typename T>
inline bool isinf(T value) {
	return std::numeric_limits<T>::has_infinity &&
		value == std::numeric_limits<T>::infinity();
}

/*	Check if an image is changing over time.
	Gets:
		(IplImage*)img	the current frame
	Returns:
		(bool)	whether or not the image has changed from the last call */
bool isMoving(IplImage *img);

/*	Returns the L2 distance between points.
	Gets:
		(CvPoint)p1		the first point
		(CvPoint)p2		the second point
	Returns:
		(double)	the distance between the points */
double dist(CvPoint p1, CvPoint p2);
double dist(CvPoint2D32f p1, CvPoint2D32f p2);

class CompareDist {
	CvPoint2D32f fixed_p;
public:
	CompareDist(CvPoint2D32f foo) {
		fixed_p.x = foo.x;
		fixed_p.y = foo.y;
	}

	bool operator()(const CvPoint2D32f& x, const CvPoint2D32f& y) {
		return (dist(fixed_p, x) < dist(fixed_p, y));
	}
};

#endif