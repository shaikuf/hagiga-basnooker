#include <iostream>
#include <cv.h>
#include <highgui.h>
#include "cue.h"
#include "misc.h"

using namespace std;

/* This returns the histogram taken around a line in the given image */
CvHistogram *hueHistFromLine(IplImage *img, CvPoint p1, CvPoint p2, int width,
							 int nbins) {
	IplImage *hsv = createBlankCopy(img);
	cvCvtColor(img, hsv, CV_BGR2HSV);
	IplImage *h = createBlankCopy(hsv, 1);
	IplImage *s = createBlankCopy(hsv, 1);
	IplImage *v = createBlankCopy(hsv, 1);
	cvSplit(hsv, h, s, v, 0);

	cvReleaseImage(&s);
	cvReleaseImage(&v);
	cvReleaseImage(&hsv);

	float h_range[] = {0., 180.};
	float *ranges[] = {h_range};
	CvHistogram *hist = cvCreateHist(1, &nbins, CV_HIST_ARRAY, (float **)ranges, 1);

	IplImage *mask = createBlankCopy(img, 1);
	cvSet(mask, cvScalar(0));
	cvLine( mask, p1, p2, cvScalar(255), width);

	cvCalcHist(&h, hist, 0, mask);
	cvReleaseImage(&mask);
	cvReleaseImage(&h);

	cvNormalizeHist(hist, 1.);

	return hist;
}

/* This returns the histogram taken around the cue in a specific sample
image */
CvHistogram *cueHistogram(int width, int nbins) {
	CvRect crop = cvRect(0, 0, 1600, 850);
	CvSize size = cvSize(800, 425);

	IplImage *src = cvLoadImage("C:\\Projecton\\Test\\Testing\\"
		"Picture 22.jpg"); 
	normalize(src, crop, size);

	IplImage *gray = createBlankCopy(src, 1);
	cvCvtColor(src, gray, CV_BGR2GRAY);

	IplImage *edge = createBlankCopy(gray);
	cvCanny(gray, edge, 150, 50);

	CvMemStorage *storage = cvCreateMemStorage(0);
	CvSeq *lines = cvHoughLines2(edge, storage, CV_HOUGH_PROBABILISTIC, 0.5, 0.5*CV_PI/180, 80, 50, 50);

	// filter lines not on the board
	//filterLinesOnTable(lines);

	// filter very similar lines
	double m_thresh = 0.05;
	double n_thresh = 10;
	filterSimiliarLines(lines, m_thresh, n_thresh);
	

	CvPoint *line = (CvPoint*)cvGetSeqElem(lines, 1);

	CvHistogram *hist = hueHistFromLine(src, line[0], line[1], width, nbins);
	cvSet1D(hist->bins, 20, cvScalar(0));

	cvReleaseImage(&src)
	cvReleaseImage(&gray);
	cvReleaseImage(&edge);
	cvReleaseMemStorage(&storage);

	return hist;
}

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

		//cout<<"("<<line[0].x<<", "<<line[0].y<<") -> (";
		//cout<<line[1].x<<", "<<line[1].y<<")\n";
		//cout<<m<<" "<<n<<endl;
	}
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

/* This finds the mean slope for the given sequence of lines.
Actually it returns the average slope for the two extreme slopes.*/
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
void markCue(IplImage *src, CvPoint2D32f white_center, float white_radius,
			 double *cue_m, CvPoint *cue_cm) {
	
	
	IplImage* src_ds = cvCreateImage(cvSize(800,600), src->depth, src->nChannels);
	cvPyrDown(src, src_ds, CV_GAUSSIAN_5x5);

	findCue(src_ds, cue_m, cue_cm);

	cue_cm->x *= src->width/800;
	cue_cm->y *= src->height/600;

	static bool debug_once = true;
	if(debug_once) {
		cvNamedWindow("pyr down");
		debug_once = false;
	}
	cvShowImage("pyr down", src_ds);
	cvReleaseImage(&src_ds);

	if(cue_cm->x == 0 && cue_cm->y == 0)
		return;
	
	int cue_sign = (int)(fabs(*cue_m)/(*cue_m));
	*cue_m = fabs(*cue_m);

	double length = 100;
	CvPoint p1, p2;

	int x_sign = 1;
	if(cue_cm->x < white_center.x) {
		// if we're pointing from the left of the ball
		x_sign = -1;
	}
	int y_sign = cue_sign;

	white_radius = white_radius + 5*downscale_factor;
	p1.x = cvRound(white_center.x + x_sign*(white_radius/sqrt(*cue_m+1)));
	p1.y = cvRound(white_center.y + y_sign*(p1.x - white_center.x)*(*cue_m));

	p2.x = p1.x + x_sign*cvRound(length/sqrt(*cue_m+1));
	p2.y = p1.y + y_sign*cvRound((p2.x-p1.x)*(*cue_m));

	cvLine(src, p1, p2, cvScalar(255, 0, 0), 3);
}

/* This filters lines with not-near-enough-to-template histogram around them */
void filterLinesByHistogram(IplImage *src, CvSeq *lines, int nbins, int width,
							 double corr_thresh) {
	bool debug = false;

	CvHistogram *cueHist = cueHistogram(width, nbins);

	if(debug) {
		cvNamedWindow("Histogram", CV_WINDOW_AUTOSIZE);
		cvNamedWindow("Stick", CV_WINDOW_AUTOSIZE);
		
		cvNamedWindow("Template Histogram", CV_WINDOW_AUTOSIZE);
		drawHistogram("Template Histogram", cueHist, nbins);
	}

	for(int i = 0; i < lines->total; i++) {
		CvPoint *line = (CvPoint*)cvGetSeqElem(lines, i);

		CvHistogram *hist = hueHistFromLine(src, line[0], line[1], width,
			nbins);

		double corr = cvCompareHist(hist, cueHist, CV_COMP_CORREL);
		if(debug) {
			cout<<corr<<endl;
		}

		if(corr > corr_thresh) {
			if(debug) {
				cvLine( src, line[0], line[1], cvScalar(255), 2);
			}
		} else {
			cvSeqRemove(lines, i);
			i--;
			if(debug) {
				cvLine( src, line[0], line[1], cvScalar(0, 0, 255), 2);
			}
		}

		if(debug) {
			drawHistogram("Histogram", hist, nbins);
			cvShowImageWrapper("Stick", src);
			cvWaitKey(0);
		}

		cvReleaseHist(&hist);
	}
	cvReleaseHist(&cueHist);

	if(debug) {
		cvDestroyWindow("Histogram");
		cvDestroyWindow("Stick");
		cvDestroyWindow("Template Histogram");
	}
}

/* This finds the center-of-mass and slope of the cue in the given image */
void findCue(IplImage *src, double *cue_m, CvPoint *cue_cm, bool debug) {
	debug = true;

	static bool create_wnd = true;
	if(create_wnd && debug) {
		cvNamedWindow("gray image");
		cvNamedWindow("edge image");
		create_wnd = false;
	}

	IplImage *gray = createBlankCopy(src, 1);
	cvCvtColor(src, gray, CV_BGR2GRAY);

	IplImage *edge = createBlankCopy(gray);
	cvCanny(gray, edge, 250, 150);

	if(debug) {
		cvShowImage("gray image", gray);
		cvShowImage("edge image", edge);
	}

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
	int nbins = 30;
	int width = 5;
	double corr_thresh = .55;
	//filterLinesByHistogram(src, lines, nbins, width, corr_thresh);

	if(debug) {
		// debug -- draw the found lines
		for(int i = 0; i < lines->total; i++) {
			CvPoint* line = (CvPoint*)cvGetSeqElem(lines,i);
			cvLine(src, line[0], line[1], CV_RGB(255,0,0), 3, 8 );
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

float line2theta(double cue_m, CvPoint cue_cm, CvPoint2D32f white_center) {
	float theta;

	if(isinf(cue_m)) {
		if(cue_cm.y < white_center.y)
			theta = PI/2;
		else
			theta = 3*PI/2;
	} else {
		cue_m = fabs(cue_m);
		if(cue_cm.x < white_center.x) {
			if(cue_cm.y < white_center.y) {
				// quarter 2
				cue_m *= -1;
				cout<<"quarter 2\n";
				theta = PI + atan(cue_m);
			} else {
				// quarter 3
				cout<<"quarter 3\n";
				theta = PI + atan(cue_m);
			}
		} else {
			if(cue_cm.y < white_center.y) {
				// quarter 1
				cout<<"quarter 1\n";
				theta = atan(cue_m);
			} else {
				// quarter 4
				cue_m *= -1;
				cout<<"quarter 4\n";
				theta = atan(cue_m) + 2*PI;
			}
		}
	}

	theta += PI;
	if(theta >= 2*PI)
		theta -= 2*PI;

	return theta;
}