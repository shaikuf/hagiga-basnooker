#include <cv.h>
#include <highgui.h>
#include <iostream>
#include "balls.h"
#include "misc.h"
#include <vector>

using namespace std;

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

		// length of the vector from the bottom-left corner to the center
	double L = sqrt(pow((double)center.x-p3.x, 2) + pow((double)center.y-p3.y, 2));

	double A = L*cos(alpha-theta);
	double B = L*sin(alpha-theta);

	res.x = (float)(A/X);
	res.y = 1-(float)(1+B/Y);

	return res;
}

/* This finds all the balls at once */
void findBalls(IplImage *img, IplImage *ball_templates[],
						 int ball_counts[], bool ball_inv_templ[],
						 vector<CvPoint> ball_centers[],
						 int n_balls) {

	IplImage *img_copy = cvCloneImage(img);
							 
	IplImage *img_inv_copy = createBlankCopy(img_copy);
	cvCvtColor(img_copy, img_inv_copy, CV_BGR2YCrCb);
	cvNot(img_inv_copy, img_inv_copy);
	cvCvtColor(img_inv_copy, img_inv_copy, CV_YCrCb2BGR);
	paintHolesBlack(img_inv_copy);

	cvSetImageROI(img_copy, tableBordersBoundingRect());
	cvSetImageROI(img_inv_copy, tableBordersBoundingRect());

	for(int i=0; i<n_balls; i++) {
		// find the template
		vector<CvPoint> p;

		if(FIND_TEMPL_DEBUG)
			cout<<"Finding "<<i<<endl;
		if(ball_inv_templ[i]) {
			p = findTemplate(img_inv_copy, ball_templates[i], BALL_CORR_THD, ball_counts[i], true);
		} else {
			p = findTemplate(img_copy, ball_templates[i], BALL_CORR_THD, ball_counts[i]);
		}

		// paint the found balls black
		for(int j=0; j<p.size(); j++) {
			cvCircle(img_copy, p[j], 1, cvScalar(0), BALL_DIAMETER*3/4);
			cvCircle(img_inv_copy, p[j], 1, cvScalar(0), BALL_DIAMETER*3/4);
		}

		// set on the results array
		for(int j=0; j<p.size(); j++) {
			p[j].x += tableBordersBoundingRect().x;
			p[j].y += tableBordersBoundingRect().y;
		}
		ball_centers[i] = p;
	}

	cvReleaseImage(&img_copy);
	cvReleaseImage(&img_inv_copy);
}