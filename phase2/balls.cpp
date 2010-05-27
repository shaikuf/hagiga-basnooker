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
						 double ball_thds[], vector<CvPoint> ball_centers[],
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
			p = findTemplate(img_inv_copy, ball_templates[i], ball_thds[i], ball_counts[i], true);
		} else {
			p = findTemplate(img_copy, ball_templates[i], ball_thds[i], ball_counts[i]);
		}

		// paint the found balls black
		for(unsigned int j=0; j<p.size(); j++) {
			cvCircle(img_copy, p[j], 1, cvScalar(0), BALL_DIAMETER*3/4);
			cvCircle(img_inv_copy, p[j], 1, cvScalar(0), BALL_DIAMETER*3/4);
		}

		// set on the results array
		for(unsigned int j=0; j<p.size(); j++) {
			p[j].x += tableBordersBoundingRect().x;
			p[j].y += tableBordersBoundingRect().y;
		}
		ball_centers[i] = p;
	}

	cvReleaseImage(&img_copy);
	cvReleaseImage(&img_inv_copy);
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
			cout<<"corr: "<<max_val<<" vs "<<corr_thd<<endl;

		if(max_val < corr_thd) {
			i = max_count;
		} else {
			if(custom_norm) {
				double mean = cvAvg(match).val[0];
				
				if(FIND_TEMPL_DEBUG) {
					cout<<"mean diff = "<<max_val-mean<<endl;
				}

				if(max_val-mean < corr_thd) { // no black ball. not much variance
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