#include <iostream>
#include <cv.h>
#include <highgui.h>
#include "cue.h"
#include "misc.h"

using namespace std;

/* This filters lines with very similiar slope and position */
void filterSimiliarLines(CvSeq *lines, double m_thresh, double n_thresh) {
	for(int i = 0; i < lines->total; i++) {
		CvPoint *line = (CvPoint*)cvGetSeqElem(lines, i);

		double m = ((double)(line[1].y - line[0].y + 0.))/(line[1].x - line[0].x);
		double n = (double)line[0].y - m*line[0].x;

		for(int j = i+1; j < lines->total; j++) {
			CvPoint *line_j = (CvPoint*)cvGetSeqElem(lines, j);
			double m_j = ((double)(line_j[1].y - line_j[0].y + 0.))/
				(line_j[1].x - line_j[0].x);
			double n_j = (double)line_j[0].y - m*line_j[0].x;

			if(fabs(m_j - m) <= m_thresh && fabs(n_j - n) <= n_thresh) {
				cvSeqRemove(lines, j);
				j--;
			}
		}
	}
}

void filterLinesByWhiteBall(CvSeq *lines, CvPoint2D32f white_center, double radius) {
	/* TODO */
}


/* This filters lines which are completely outside tableBorders() */
void filterLinesOnTable(CvSeq *lines) {
	// the contour of the board playable area
	CvMemStorage *mem = cvCreateMemStorage();
	CvSeq *borders = tableBorders(mem);

	for(int i = 0; i < lines->total; i++)
    {
		CvPoint *line = (CvPoint*)cvGetSeqElem(lines, i);

		if(!(cvPointPolygonTest(borders, cvPointTo32f(line[0]), false) > 0 ||
			cvPointPolygonTest(borders, cvPointTo32f(line[1]), false) > 0)) {
			cvSeqRemove(lines, i);
			i--;
		}
    }
	
	cvReleaseMemStorage(&mem);
}

/* This finds the mean slope the two extreme slopes.*/
void meanLine(CvSeq *lines, double *dst_m) {
	int max_i, min_i;
	double max_m = 0, min_m = 0;
	for(int i = 0; i < lines->total; i++) {
		CvPoint *line = (CvPoint*)cvGetSeqElem(lines, i);
		
		double m = ((double)(line[1].y - line[0].y + 0.))/(line[1].x - line[0].x);
		if(i == 0) {
			max_m = min_m = m;
			max_i = min_i = 0;
		} else {
			if(m > max_m) {
				max_m = m;
				max_i = i;
			} else if(m < min_m) {
				min_m = m;
				min_i = i;
			}
		}
	}

	if(!(min_m >= DBL_MIN && min_m <= DBL_MAX)) {
		*dst_m = max_m;
	} else if(!(max_m >= DBL_MIN && max_m <= DBL_MAX)) {
		*dst_m = min_m;
	} else {
		*dst_m = (min_m + max_m)/2;
	}
}

/* This finds the mean center-of-mass for the given sequence of lines */
void meanCM(CvSeq *lines, CvPoint *dst_cm) {
	double cm_x = 0, cm_y = 0;

	if(lines->total == 0) {
		dst_cm->x = 0;
		dst_cm->y = 0;
		return;
	}

	for(int i = 0; i < lines->total; i++) {
		CvPoint *line = (CvPoint*)cvGetSeqElem(lines, i);
		
		cm_x += line[0].x + line[1].x;
		cm_y += line[0].y + line[1].y;
	}

	dst_cm->x = (int)(cm_x/(2*lines->total));
	dst_cm->y = (int)(cm_y/(2*lines->total));
}

/* This uses findCue() and marks the results on the image (around the white
ball) */
void findAndMarkCue(IplImage *src, CvPoint2D32f white_center, float white_radius,
			 double *cue_m, CvPoint *cue_cm) {
	
	// downsample the image
	IplImage* src_ds = cvCreateImage(cvSize(CUE_PYRDOWN_WIDTH, CUE_PYRDOWN_HEIGHT),
		src->depth, src->nChannels);
	cvPyrDown(src, src_ds, CV_GAUSSIAN_5x5);

	// find the cue
	findCue(src_ds, cue_m, cue_cm, white_center);

	// fix the coordinates
	cue_cm->x *= src->width/CUE_PYRDOWN_WIDTH;
	cue_cm->y *= src->height/CUE_PYRDOWN_HEIGHT;

	// show debug image
	static bool debug_once = CUE_MARK_DEBUG;
	if(debug_once) {
		cvNamedWindow("pyr down");
		debug_once = false;
	}
	if(CUE_MARK_DEBUG) {
		cvShowImage("pyr down", src_ds);
		cvReleaseImage(&src_ds);
	}

	// if we couldn't find the cue stick
	if(cue_cm->x == 0 && cue_cm->y == 0)
		return;
	
	// draw blue line on src
	if(CUE_MARK_DEBUG) {
		double cue_m_abs = fabs(*cue_m);
		int cue_sign = (int)(cue_m_abs/(*cue_m));

		double length = 100;
		CvPoint p1, p2;

		int x_sign = 1;
		if(cue_cm->x < white_center.x) {
			// if we're pointing from the left of the ball
			x_sign = -1;
		}
		int y_sign = cue_sign;

		white_radius = white_radius + 5;
		p1.x = cvRound(white_center.x + x_sign*(white_radius/sqrt(*cue_m+1)));
		p1.y = cvRound(white_center.y + y_sign*(p1.x - white_center.x)*(*cue_m));

		p2.x = p1.x + x_sign*cvRound(length/sqrt(*cue_m+1));
		p2.y = p1.y + y_sign*cvRound((p2.x-p1.x)*(*cue_m));

		cvLine(src, p1, p2, cvScalar(255, 0, 0), 3);
	}
}

/* This finds the center-of-mass and slope of the cue in the given image */
void findCue(IplImage *src, double *cue_m, CvPoint *cue_cm, CvPoint2D32f white_center) {
	static bool first_debug = CUE_FIND_DEBUG;
	if(first_debug) {
		cvNamedWindow("gray image");
		cvNamedWindow("edge image");
		first_debug = false;
	}

	// create grayscale image
	IplImage *gray = createBlankCopy(src, 1);
	cvCvtColor(src, gray, CV_BGR2GRAY);

	// create edge image
	IplImage *edge = createBlankCopy(gray);
	cvCanny(gray, edge, 250, 150);

	if(CUE_FIND_DEBUG) {
		cvShowImage("gray image", gray);
		cvShowImage("edge image", edge);
	}

	// find lines
	CvMemStorage *storage = cvCreateMemStorage(0);
	CvSeq *lines = cvHoughLines2(edge, storage, CV_HOUGH_PROBABILISTIC, 0.5, 0.5*CV_PI/180, 30, 30, 5);

	// filter lines not on the board
	drawBorders(src, 1);
	filterLinesOnTable(lines);

	// filter very similar lines
	double m_thresh = 0.05;
	double n_thresh = 10;
	filterSimiliarLines(lines, m_thresh, n_thresh);

	// filter the cue using histogram
	/*int nbins = 30;
	int width = 5;
	double corr_thresh = .55;
	filterLinesByHistogram(src, lines, nbins, width, corr_thresh);*/

	// filter by white ball
	/*double radius = 15;
	filterLinesByWhiteBall(lines, white_center, radius);*/

	if(CUE_FIND_DEBUG) {
		// draw the filtered lines
		for(int i = 0; i < lines->total; i++) {
			CvPoint* line = (CvPoint*)cvGetSeqElem(lines,i);
			cvLine(src, line[0], line[1], CV_RGB(0,0,255), 3, 8 );
		}
	}

	// compute the mean line
	meanLine(lines, cue_m);
	// compute the center of mass
	meanCM(lines, cue_cm);

	cvReleaseImage(&gray);
	cvReleaseImage(&edge);
	cvReleaseMemStorage(&storage);
}

/* Convert the line from center of mass and slope to theta */
double line2theta(double cue_m, CvPoint cue_cm, CvPoint2D32f white_center) {
	double theta;

	if(isinf(cue_m)) { // perpendicular line
		if(cue_cm.y < white_center.y)
			theta = PI/2;
		else
			theta = 3*PI/2;

	} else { // non-perpendicular line
		cue_m = fabs(cue_m);
		if(cue_cm.x < white_center.x) {
			if(cue_cm.y < white_center.y) {
				// quarter 2
				cue_m *= -1;
				theta = PI + atan(cue_m);
			} else {
				// quarter 3
				theta = PI + atan(cue_m);
			}
		} else {
			if(cue_cm.y < white_center.y) {
				// quarter 1
				theta = atan(cue_m);
			} else {
				// quarter 4
				cue_m *= -1;
				theta = atan(cue_m) + 2*PI;
			}
		}
	}

	theta += PI;
	if(theta >= 2*PI)
		theta -= 2*PI;

	return theta;
}