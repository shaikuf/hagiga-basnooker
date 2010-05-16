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

	if(invert) {
		IplImage *tmp = createBlankCopy(img);
		cvCvtColor(img, tmp, CV_BGR2YCrCb);
		cvNot(tmp, tmp);
		cvCvtColor(tmp, tmp, CV_YCrCb2BGR);
		p = findTemplate(tmp, templ, BALL_CORR_THD, max_count);
		cvReleaseImage(&tmp);
	} else {
		p = findTemplate(img, templ, BALL_CORR_THD, max_count);
	}

	return p;
}

/* Fix an absolute position on the image, to a position relative to the table */
CvPoint2D32f fixPosition(CvPoint center) {
	
	// read the edges calibration file
	CvMemStorage *mem = cvCreateMemStorage();

	char filename[100];
	_snprintf_s(filename, 100, "edges-%d.xml", 0);
	CvSeq* edges = (CvSeq*)cvLoad(filename, mem);

	CvPoint p0 = *(CvPoint*)cvGetSeqElem(edges, 0); // top-left
	CvPoint p1 = *(CvPoint*)cvGetSeqElem(edges, 1); // top-right
	CvPoint p2 = *(CvPoint*)cvGetSeqElem(edges, 2); // bottom-right
	CvPoint p3 = *(CvPoint*)cvGetSeqElem(edges, 3); // bottom-left

	cvReleaseMemStorage(&mem);

	// fix the position

	CvPoint2D32f res;

		// dimensions of the rectangle
	double X = sqrt(pow((double)p3.x-p2.x,2)+pow((double)p3.y-p2.y,2));
	double Y = sqrt(pow((double)p3.x-p0.x,2)+pow((double)p3.y-p0.y,2));

		// angles of the rectangle
			// angle of the bottom against the horizon
	double theta = atan2((double)p2.y-p3.y, p2.x-p3.x);
			// angle of the vector from the bottom-left corner to the center
	double alpha = atan2((double)center.y-p3.y, (double)center.x-p3.x);

	if(DEBUG_FIX_POS) {
			// angle of the top against the horizon
		double theta_tag = atan2((double)p1.y-p0.y, (double)p1.x-p0.x);
			// angle of the left side against the horizon
		double betta = atan2((double)p0.y-p3.y, p0.x-p3.x);
			// angle of the right side against the horizon
		double betta_tag = atan2((double)p1.y-p2.y, p1.x-p2.x);
		cout<<"bottom="<<theta*180/PI<<"\n"<<"top="<<theta_tag*180/PI<<"\n"
			<<"left="<<betta*180/PI<<"\n"<<"right="<<betta_tag*180/PI<<endl;
		cout<<"=========================="<<endl;
	}
	
		// length of the vector from the bottom-left corner to the center
	double L = sqrt(pow((double)center.x-p3.x, 2) + pow((double)center.y-p3.y, 2));

	double A = L*cos(alpha-theta);
	double B = L*sin(alpha-theta);

	res.x = (float)(A/X);
	res.y = (float)(1+B/Y);

	return res;
}