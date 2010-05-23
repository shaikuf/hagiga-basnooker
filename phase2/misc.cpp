#include <cv.h>
#include <highgui.h>
#include "misc.h"
#include <iostream>
#include <vector>

using namespace std;

void cvMatchTemplate2(IplImage *I, IplImage *T, IplImage *M, int foo) {
	int x, y, x_tag, y_tag;
	double val, Z_I, Z_T;
	CvScalar s_I, s_T;

	for(x=0; x <200; x++) {
		for(y=0; y<200; y++) {
			val = 0;
			Z_I = 0;
			Z_T = 0;
			for(x_tag = 0; x_tag < T->width; x_tag++) {
				for(y_tag = 0; y_tag < T->height; y_tag++) {
					s_I = cvGet2D(I, y+y_tag, x+x_tag);
					s_T = cvGet2D(T, y_tag, x_tag);
					val = s_I.val[0]*s_T.val[0]*s_I.val[0]*s_T.val[0];
					Z_I = s_I.val[0]*s_I.val[0];
					Z_T = s_T.val[0]*s_T.val[0];
				}
			}
			cvSet2D(M, y, x, cvScalar(val/pow(Z_I*Z_T, 0.5)));
		}
	}

	for(; x<M->width; x++) {
		for(; y<M->height; y++) {
			cvSet2D(M, y, x, cvScalar(val/pow(Z_I*Z_T, 0.5)));
		}
	}
}

/* Finds the point which best matches the template */
vector<CvPoint> findTemplate(IplImage *img, IplImage *templ, double corr_thd,
							 int max_count, bool custom_norm) {

	if(custom_norm) {
		IplImage *I_gray = createBlankCopy(img, 1);
		IplImage *T_gray = createBlankCopy(templ, 1);
		cvCvtColor(img, I_gray, CV_BGR2GRAY);
		cvCvtColor(templ, T_gray, CV_BGR2GRAY);
		img = I_gray;
		templ = T_gray;
	}

	CvSize img_s = cvGetSize(img);
	CvSize templ_s = cvGetSize(templ);

	// create an image the size the correlation function return
	IplImage *match = cvCreateImage(cvSize(img_s.width - templ_s.width + 1,
		img_s.height - templ_s.height + 1), IPL_DEPTH_32F, 1);

	// get the correlation image
	if(!custom_norm) {
		cvMatchTemplate(img, templ, match, CV_TM_CCORR_NORMED);
	} else {
		cvMatchTemplate(img, templ, match, CV_TM_CCORR);
		
		// limit the image above 0
		IplImage *limit_mask = cvCreateImage(cvSize(img_s.width - templ_s.width + 1,
			img_s.height - templ_s.height + 1), IPL_DEPTH_8U, 1);
		cvCmpS(match, 0, limit_mask, CV_CMP_LT);
		cvSet(match, cvScalar(0), limit_mask);
		cvReleaseImage(&limit_mask);

		// shift and scale
		double tmp_max, tmp_min;
		cvMinMaxLoc(match, &tmp_min, &tmp_max, 0, 0, 0);
		cvAddS(match, cvScalar(-1*tmp_min), match);
		cvConvertScale(match, match, 1/(tmp_max-tmp_min), 0);
	}

	if(FIND_TEMPL_DEBUG) {
		cvNamedWindow("Original Image", CV_WINDOW_AUTOSIZE);
		cvShowImage("Original Image", img);
	}

	vector<CvPoint> res;

	// find the maximal correlation
	for(int i = 0; i < max_count; i++) {
		CvPoint max_p;
		double max_val;
		cvMinMaxLoc(match, 0, &max_val, 0, &max_p, 0);

		if(FIND_TEMPL_DEBUG) {
			cvNamedWindow("Template", CV_WINDOW_AUTOSIZE);
			cvShowImage("Template", templ);
			
			// paint the corr. and the circle on match_draw
			IplImage *match_draw = createBlankCopy(match, 3);
			cvCvtColor(match, match_draw, CV_GRAY2BGR);
			cvCircle(match_draw, max_p, 1, cvScalar(0xff, 0), 2);

			cvNamedWindow("Template matching", CV_WINDOW_AUTOSIZE);
			cvShowImage("Template matching", match_draw);
			cvWaitKey(0);
			cvDestroyWindow("Template matching");
			cvDestroyWindow("Template");
			cvDestroyWindow("Original Image");

			cvReleaseImage(&match_draw);
		}

		if(FIND_TEMPL_DEBUG)
			cout<<"corr: "<<max_val<<endl;

		if(max_val < corr_thd) {
			i = max_count;
		} else {
			if(custom_norm) {
				double mean = cvAvg(match).val[0];
				
				if(FIND_TEMPL_DEBUG) {
					cout<<"max = "<<max_val<<endl;
					cout<<"mean = "<<mean<<endl;
					cout<<"diff = "<<max_val-mean<<endl;
				}

				if(mean > 0.85) { // no black ball. not much variance
					i = max_count;
					continue;
				}
			}
			
			cvCircle(match, max_p, 1, cvScalar(0), BALL_DIAMETER);

			// fix the position of the maximal correlation to be the middle of the
			// template
			max_p.x = max_p.x + (templ->width)/2;
			max_p.y = max_p.y + (templ->height)/2;
			
			res.push_back(max_p);
		}
	}

	cvReleaseImage(&match);
	if(custom_norm) {
		cvReleaseImage(&img);
		cvReleaseImage(&templ);
	}

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
		_snprintf_s(filename, 100, "edges-%d.xml", 0);
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

/* this prints the angles of the edges calibration */
void printBorderAngles() {
	CvSeq *borders = tableBorders();

	// print the angles of the borders
	CvPoint p0 = *(CvPoint*)cvGetSeqElem(borders, 0); // top-left
	CvPoint p1 = *(CvPoint*)cvGetSeqElem(borders, 1); // top-right
	CvPoint p2 = *(CvPoint*)cvGetSeqElem(borders, 2); // bottom-right
	CvPoint p3 = *(CvPoint*)cvGetSeqElem(borders, 3); // bottom-left

		// angle of the bottom against the horizon
	double theta = atan2((double)p2.y-p3.y, p2.x-p3.x);
		// angle of the top against the horizon
	double theta_tag = atan2((double)p1.y-p0.y, (double)p1.x-p0.x);
		// angle of the left side against the horizon
	double betta = atan2((double)p0.y-p3.y, p0.x-p3.x);
		// angle of the right side against the horizon
	double betta_tag = atan2((double)p1.y-p2.y, p1.x-p2.x);
	cout<<"bottom="<<theta*180/PI<<"\n"<<"top="<<theta_tag*180/PI<<"\n"
		<<"left="<<betta*180/PI<<"\n"<<"right="<<betta_tag*180/PI<<endl;
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

/* This marks a cross on the image */
void markCross(IplImage *img, CvPoint center, CvScalar color) {

	// print an overlay image of the found circle
	IplImage *overlay_drawing = createBlankCopy(img, 1);
	cvSet(overlay_drawing, cvScalar(0));

	// draw the cross on the overlay
    int line_len = 5;
	cvLine(overlay_drawing, cvPoint(cvRound(center.x)-line_len,
		cvRound(center.y)), cvPoint(cvRound(center.x)+line_len,
		cvRound(center.y)), cvScalar(0xff), 1);
	cvLine(overlay_drawing, cvPoint(cvRound(center.x),
		cvRound(center.y)-line_len), cvPoint(cvRound(center.x),
		cvRound(center.y)+line_len), cvScalar(0xff), 1);

	// draw the overlay on the image
	cvSet(img, color, overlay_drawing);

	cvReleaseImage(&overlay_drawing);
}

/* paints the holes of the table black */
void paintHolesBlack(IplImage *img) {
	CvSeq *borders = tableBorders();

	// print the angles of the borders
	CvPoint p0 = *(CvPoint*)cvGetSeqElem(borders, 0); // top-left
	CvPoint p1 = *(CvPoint*)cvGetSeqElem(borders, 1); // top-right
	CvPoint p2 = *(CvPoint*)cvGetSeqElem(borders, 2); // bottom-right
	CvPoint p3 = *(CvPoint*)cvGetSeqElem(borders, 3); // bottom-left
	CvPoint p4 = cvPoint((p0.x + p1.x)/2, (p0.y + p1.y)/2);
	CvPoint p5 = cvPoint((p2.x + p3.x)/2, (p2.y + p3.y)/2);

	int delta = 5;
	p0 = cvPoint(p0.x-delta, p0.y-delta);
	p1 = cvPoint(p1.x+delta, p1.y-delta);
	p2 = cvPoint(p2.x+delta, p2.y+delta);
	p3 = cvPoint(p3.x-delta, p3.y+delta);
	p4 = cvPoint(p4.x, p4.y-4.5*delta);
	p5 = cvPoint(p5.x, p5.y+4.5*delta);

	int size = 30;
	cvCircle(img, p0, size, cvScalar(0), -1);
	cvCircle(img, p1, size, cvScalar(0), -1);
	cvCircle(img, p2, size, cvScalar(0), -1);
	cvCircle(img, p3, size, cvScalar(0), -1);
	cvCircle(img, p4, size, cvScalar(0), -1);
	cvCircle(img, p5, size, cvScalar(0), -1);
}

CvRect tableBordersBoundingRect() {
	CvSeq *borders = tableBorders();

	return cvBoundingRect(borders);
}
