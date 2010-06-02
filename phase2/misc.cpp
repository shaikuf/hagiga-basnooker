#include <cv.h>
#include <highgui.h>
#include "misc.h"
#include <iostream>
#include <vector>

using namespace std;

CvSeq *tableBorders() {
	static bool once = true;
	static CvMemStorage *mem;
	static CvSeq* borders;

	if(once) {
		// read from file only once
		mem = cvCreateMemStorage();

		char filename[100];
		_snprintf_s(filename, 100, "edges-%d.xml", 0);
		borders = (CvSeq*)cvLoad(filename, mem);

		once = false;
	}

	// and return it always
	return borders;
}

void drawBorders(IplImage *dst, int width) {
	CvSeq *borders = tableBorders();

	for(int i = 0; i < borders->total; i++) {
		CvPoint p1 = *(CvPoint*)cvGetSeqElem(borders, i);
		CvPoint p2 = *(CvPoint*)cvGetSeqElem(borders, (i+1)%borders->total);

		cvLine(dst, p1, p2, cvScalar(0, 255, 0), width);
	}
}

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

IplImage *createBlankCopy(IplImage *src, int channels, int depth) {
	if(depth == -1)
		depth = src->depth;
	if(channels == -1)
		channels = src->nChannels;

	return cvCreateImage(cvGetSize(src), depth, channels);
}

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

void paintHolesBlack(IplImage *img) {
	static CvPoint p[6];
	static int radii[6];

	static bool once = true;
	if(once) {
		CvMat* holes = (CvMat*)cvLoad("Hole.xml");
		for(int i=0; i<6; i++){
			p[i].x = CV_MAT_ELEM(*holes,int,i,0);
			p[i].y = CV_MAT_ELEM(*holes,int,i,1);
			radii[i] = CV_MAT_ELEM(*holes,int,i,2);
		}
		once = false;
	}

	for(int i=0; i<6; i++)
			cvCircle(img, p[i], radii[i], cvScalar(0), -1);
}

CvRect tableBordersBoundingRect(int offset) {
	CvSeq *borders = tableBorders();

	// actually we return a rect with BOUNDING_RECT_OFFSET more on each side
	CvRect bound = cvBoundingRect(borders);
	return cvRect(bound.x-offset/2, bound.y-offset/2,
		bound.width+offset, bound.height+offset);
}

bool isMoving(IplImage *img) {
	// always keep a gray copy of the last frame
	static IplImage *last_frame = 0;

	if(last_frame == 0) {
		// if it's the first time, allocate it and return
		last_frame = createBlankCopy(img, 1);
		cvCvtColor(img, last_frame, CV_BGR2GRAY);
		return false;
	}

	// create a difference image of the gray images
	IplImage *img_gray = createBlankCopy(last_frame);
	IplImage *diff = createBlankCopy(last_frame);
	cvCvtColor(img, img_gray, CV_BGR2GRAY);
	cvAbsDiff(img_gray, last_frame, diff);
	// threshold it
	cvThreshold(diff, diff, 20, 255, CV_THRESH_BINARY);

	if(IS_MOVING_DEBUG) {
		cvNamedWindow("Diff");
		cvShowImage("Diff", diff);
	}

	// update the last_frame
	cvCopy(img_gray, last_frame);

	// see if we have movement on any pixel
	double max;
	cvMinMaxLoc(diff, 0, &max, 0, 0);

	cvReleaseImage(&img_gray);
	cvReleaseImage(&diff);

	return (max > 0);
}

double dist(CvPoint p1, CvPoint p2) {
	return pow((double)((p1.x - p2.x)*(p1.x - p2.x) +
		(p1.y - p2.y)*(p1.y - p2.y)), (double)0.5);
}

double dist(CvPoint2D32f p1, CvPoint2D32f p2) {
	return pow((double)((p1.x - p2.x)*(p1.x - p2.x) +
		(p1.y - p2.y)*(p1.y - p2.y)), (double)0.5);
}
