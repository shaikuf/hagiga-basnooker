#include <cv.h>
#include <highgui.h>
#include <iostream>
#include "balls.h"
#include "misc.h"
#include <vector>

using namespace std;

/* This finds the ball matching the given template on the image */
vector<CvPoint> findBall(IplImage *img, IplImage *templ, int max_count,
						 bool invert) {

	// find the template
	vector<CvPoint> p;

	double corr_thd = 0.7;
	if(invert) { /* TODO -- debug */
		IplImage *tmp = createBlankCopy(img);
		cvCvtColor(img, tmp, CV_BGR2YCrCb);
		cvNot(tmp, tmp);
		cvCvtColor(tmp, tmp, CV_YCrCb2BGR);
		p = findTemplate(tmp, templ, corr_thd, max_count);
		cvReleaseImage(&tmp);
	} else {
		p = findTemplate(img, templ, corr_thd, max_count);
	}

	return p;
}

/* This uses findBall() and marks the results on the image */
void markBall(IplImage *img, CvPoint center, CvScalar color) {

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

/* Fix an absolute position on the image, to a position relative to the table */
CvPoint2D32f fixPosition(CvPoint2D32f center) {
	
	// read the edges calibration file
	CvMemStorage *mem = cvCreateMemStorage();

	char filename[100];
	_snprintf_s(filename, 100, "edges-%d.xml", 0);
	CvSeq* edges = (CvSeq*)cvLoad(filename, mem);

	CvPoint p0 = *(CvPoint*)cvGetSeqElem(edges, 0);
	CvPoint p3 = *(CvPoint*)cvGetSeqElem(edges, 3);
	CvPoint p2 = *(CvPoint*)cvGetSeqElem(edges, 2);

	cvReleaseMemStorage(&mem);

	// fix the position
	double X = sqrt(pow((double)p3.x-p2.x,2)+pow((double)p3.y-p2.y,2));
	double Y = sqrt(pow((double)p3.x-p0.x,2)+pow((double)p3.y-p0.y,2));

	double theta = atan2((double)p2.y-p3.y, p2.x-p3.x);
	double alpha = atan2(center.y-p3.y, center.x-p3.x);

	cout<<"theta="<<theta*180/PI<<"\n"<<"alpha="<<alpha*180/PI<<"\n";
	
	double L = sqrt(pow(center.x-p3.x, 2) + pow(center.y-p3.y, 2));

	double A = L*cos(alpha-theta);
	double B = L*sin(alpha-theta);

	CvPoint2D32f res;

	res.x = (float)(A/X);
	res.y = (float)(1+B/Y);

	return res;
}