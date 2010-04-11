#ifndef _CUE_H
#define _CUE_H

#include <cv.h>

/* This uses findCue() and marks the results on the image (around the white
ball) */
void markCue(IplImage *src, CvPoint2D32f white_center, float white_radius,
			 double *cue_m, CvPoint *cue_cm);

/* This finds the center-of-mass and slope of the cue in the given image */
void findCue(IplImage *src, double *cue_m, CvPoint *cue_cm, bool debug = false);

/* This filters lines which are completely outside tableBorders() */
void filterLinesOnTable(CvSeq *lines);

/* This filters lines with very similiar slope and position */
void filterSimiliarLines(CvSeq *lines, double m_thresh, double n_thresh);

/* This filters lines with not-near-enough-to-template histogram around them */
void filterLinesByHistogram(IplImage *src, CvSeq *lines, int nbins, int width,
							 double corr_thresh);

/* This returns the histogram taken around a line in the given image */
CvHistogram *hueHistFromLine(IplImage *img, CvPoint p1, CvPoint p2, int width,
							 int nbins);

/* This returns the histogram taken around the cue in a specific sample
image */
CvHistogram *cueHistogram(int width, int nbins);

/* This finds the mean slope for the given sequence of lines.
Actually it returns the average slope for the two extreme slopes.*/
void meanLine(CvSeq *lines, double *dst_m);

/* This finds the mean center-of-mass for the given sequence of lines */
void meanCM(CvSeq *lines, CvPoint *dst_cm);

#endif