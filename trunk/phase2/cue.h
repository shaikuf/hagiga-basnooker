#ifndef _CUE_H
#define _CUE_H

#include <cv.h>

#define CUE_PYRDOWN_WIDTH 800
#define CUE_PYRDOWN_HEIGHT 600

#define CUE_FIND_DEBUG 1
#define CUE_MARK_DEBUG 1
#define CUE_HIST_DEBUG 0

/* This uses findCue() and marks the results on the image (around the white
ball) */
void findAndMarkCue(IplImage *src, CvPoint2D32f white_center, float white_radius,
			 double *cue_m, CvPoint *cue_cm);

/* This finds the center-of-mass and slope of the cue in the given image */
void findCue(IplImage *src, double *cue_m, CvPoint *cue_cm, CvPoint2D32f white_center);

/* This filters lines which are completely outside tableBorders() */
void filterLinesOnTable(CvSeq *lines);

/* This filters lines with very similiar slope and position */
void filterSimiliarLines(CvSeq *lines, double m_thresh, double n_thresh);

/* This filters lines that are not passing through the white ball */
void filterLinesByWhiteBall(CvSeq *lines, CvPoint2D32f white_center, double radius);

/* This finds the mean slope the two extreme slopes.*/
void meanLine(CvSeq *lines, double *dst_m);

/* This finds the mean center-of-mass for the given sequence of lines */
void meanCM(CvSeq *lines, CvPoint *dst_cm);

/* Convert the line from center of mass and slope to theta */
double line2theta(double cue_m, CvPoint cue_cm, CvPoint2D32f white_center);

/* Find the angle of the cue using the white markers */
void findCueWithWhiteMarkers(IplImage *src, double *theta);

#endif