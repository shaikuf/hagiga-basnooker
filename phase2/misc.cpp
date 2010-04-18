#include <cv.h>
#include <highgui.h>
#include "misc.h"

using namespace std;

/* Finds the point which best matches the template */
void findTemplate(IplImage *img, IplImage *templ, CvPoint *p) {

	CvSize img_s = cvGetSize(img);
	CvSize templ_s = cvGetSize(templ);

	// create an image the size the correlation function return
	IplImage *match = cvCreateImage(cvSize(img_s.width - templ_s.width + 1,
		img_s.height - templ_s.height + 1), IPL_DEPTH_32F, 1);

	// get the correlation image
	cvMatchTemplate(img, templ, match, CV_TM_CCORR_NORMED);

	// find the maximal correlation
	CvPoint max_p;
	cvMinMaxLoc(match, 0, 0, 0, &max_p, 0);

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

	// fix the position of the maximal correlation to be the middle of the
	// template
	p->x = max_p.x + (templ->width)/2;
	p->y = max_p.y + (templ->height)/2;

	cvReleaseImage(&match);
}

/* This returns a sequence of CvPoints specifying the contour of the
board borders */
CvSeq *tableBorders(CvMemStorage *mem) {
	char filename[100];
	_snprintf_s(filename, 100, "borders-%d.xml", 0);
	CvSeq* borders = (CvSeq*)cvLoad(filename, mem);

	return borders;
}

/* This finds and draws the borders of the table */
void drawBorders(IplImage *dst, int width) {
	CvMemStorage *mem = cvCreateMemStorage();
	CvSeq *borders = tableBorders(mem);

	for(int i = 0; i < borders->total; i++) {
		CvPoint p1 = *(CvPoint*)cvGetSeqElem(borders, i);
		CvPoint p2 = *(CvPoint*)cvGetSeqElem(borders, (i+1)%borders->total);

		cvLine(dst, p1, p2, cvScalar(0, 255, 0), width);
	}

	cvReleaseMemStorage(&mem);
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
