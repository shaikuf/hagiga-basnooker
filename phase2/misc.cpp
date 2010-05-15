#include <cv.h>
#include <highgui.h>
#include "misc.h"
#include <iostream>
#include <vector>

using namespace std;

/* Finds the point which best matches the template */
vector<CvPoint> findTemplate(IplImage *img, IplImage *templ, double corr_thd,
							 int max_count) {

	CvSize img_s = cvGetSize(img);
	CvSize templ_s = cvGetSize(templ);

	// create an image the size the correlation function return
	IplImage *match = cvCreateImage(cvSize(img_s.width - templ_s.width + 1,
		img_s.height - templ_s.height + 1), IPL_DEPTH_32F, 1);

	// get the correlation image
	cvMatchTemplate(img, templ, match, CV_TM_CCOEFF_NORMED);

	vector<CvPoint> res;

	// find the maximal correlation
	for(int i = 0; i < max_count; i++) {
		CvPoint max_p;
		double max_val;
		cvMinMaxLoc(match, 0, &max_val, 0, &max_p, 0);

		if(FIND_TEMPL_DEBUG) {
			// paint the corr. and the circle on match_draw
			IplImage *match_draw = createBlankCopy(match, 3);
			cvCvtColor(match, match_draw, CV_GRAY2BGR);
			cvCircle(match_draw, max_p, 1, cvScalar(0xff, 0), 2);

			cvNamedWindow("Template matching", CV_WINDOW_AUTOSIZE);
			cvShowImage("Template matching", match_draw);
			cvWaitKey(0);
			cvDestroyWindow("Template matching");

			cvReleaseImage(&match_draw);
		}

		if(FIND_TEMPL_DEBUG)
			cout<<"corr: "<<max_val<<endl;

		if(max_val < corr_thd) {
			i = max_count;
		} else {
			cvCircle(match, max_p, 1, cvScalar(0), BALL_DIAMETER);

			if(isPointOnTable(max_p, 0)) {
				if(FIND_TEMPL_DEBUG)
					cout<<"is in"<<endl;

				// fix the position of the maximal correlation to be the middle of the
				// template
				max_p.x = max_p.x + (templ->width)/2;
				max_p.y = max_p.y + (templ->height)/2;
				
				res.push_back(max_p);
			} else {
				i--;
			}
		}
	}

	cvReleaseImage(&match);

	return res;
}

/* This returns a sequence of CvPoints specifying the contour of the
board borders */
CvSeq *tableBorders() {
	static bool once = true;
	static CvMemStorage *mem;
	static CvSeq* borders;

	if(once) {
		mem = cvCreateMemStorage();

		char filename[100];
		_snprintf_s(filename, 100, "borders-%d.xml", 0);
		borders = (CvSeq*)cvLoad(filename, mem);
	}

	return borders;
}

/* This finds and draws the borders of the table */
void drawBorders(IplImage *dst, int width) {
	CvSeq *borders = tableBorders();

	for(int i = 0; i < borders->total; i++) {
		CvPoint p1 = *(CvPoint*)cvGetSeqElem(borders, i);
		CvPoint p2 = *(CvPoint*)cvGetSeqElem(borders, (i+1)%borders->total);

		cvLine(dst, p1, p2, cvScalar(0, 255, 0), width);
	}
}

/* Create a new IplImage, the same size of src. If channels is -1, it's also
the same number of channels. The same for depth. */
IplImage *createBlankCopy(IplImage *src, int channels, int depth) {
	if(depth == -1)
		depth = src->depth;
	if(channels == -1)
		channels = src->nChannels;

	return cvCreateImage(cvGetSize(src), depth, channels);
}

/* This filters points which are outside tableBorders() */
vector<CvPoint2D32f> filterPointsOnTable(const vector<CvPoint2D32f> &centers,
											  double max_dist) {

	CvSeq *borders = tableBorders();

	vector<CvPoint2D32f> res_points;

	vector<CvPoint2D32f>::const_iterator i;
	for(i=centers.begin(); i != centers.end(); i++) {
		if(cvPointPolygonTest(borders, *i, 1) >= -1*max_dist)
			res_points.push_back(*i);
	}

	return res_points;
}

bool isPointOnTable(CvPoint2D32f p, double max_dist) {
	CvSeq *borders = tableBorders();

	return (cvPointPolygonTest(borders, p, 1) >= -1*max_dist);
}

bool isPointOnTable(CvPoint p, double max_dist) {
	return isPointOnTable(cvPoint2D32f(p.x, p.y), max_dist);
}