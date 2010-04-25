#ifndef _MISC_H
#define _MISC_H

#include <cv.h>
#include <limits>

#define PI 3.14159265

#define SAVE_IMAGES 0
#define FIND_TEMPL_DEBUG 0

#define USE_BIRDS_EYE 1

/* Create a new IplImage, the same size of src. If channels is -1, it's also
the same number of channels. The same for depth. */
IplImage *createBlankCopy(IplImage *src, int channels = -1, int depth = -1);

/* Finds the point which best matches the template */
void findTemplate(IplImage *img, IplImage *templ, CvPoint *p);

/* This returns a sequence of CvPoints specifying the contour of the
board borders */
CvSeq *tableBorders(CvMemStorage *mem);

/* This finds and draws the borders of the table */
void drawBorders(IplImage *dst, int width);

template<typename T>
inline bool isinf(T value) {
	return std::numeric_limits<T>::has_infinity &&
		value == std::numeric_limits<T>::infinity();
}

#endif