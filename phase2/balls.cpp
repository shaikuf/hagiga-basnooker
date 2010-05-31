#include <cv.h>
#include <highgui.h>
#include <iostream>
#include "balls.h"
#include "misc.h"
#include <vector>

using namespace std;

CvPoint2D32f fixPosition(CvPoint center) {
	// get the table borders
	CvSeq* edges = tableBorders();

	CvPoint p0 = *(CvPoint*)cvGetSeqElem(edges, 0); // top-left
	CvPoint p1 = *(CvPoint*)cvGetSeqElem(edges, 1); // top-right
	CvPoint p2 = *(CvPoint*)cvGetSeqElem(edges, 2); // bottom-right
	CvPoint p3 = *(CvPoint*)cvGetSeqElem(edges, 3); // bottom-left

	// fix the position
	/* this works by assuming that the points are the edges of a rectangle
	(is a good approximation) and computes the relative X-Y inside this
	rectangle */

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

	// fix
	res.x = (float)(A/X);
	res.y = 1-(float)(1+B/Y);

	return res;
}


void findBalls(IplImage *img, IplImage *ball_templates[],
						 int ball_counts[], bool ball_inv_templ[],
						 double ball_thds[], vector<CvPoint> ball_centers[],
						 int n_balls) {

	/* this is a copy of the image we mark find balls on (in order for the
	correlation there with any other template to be small) */
	IplImage *img_copy = cvCloneImage(img);
	/* this is exactly the same only with the inverted image, to search for
	inverted templates on */
	IplImage *img_inv_copy = createBlankCopy(img_copy);
	cvCvtColor(img_copy, img_inv_copy, CV_BGR2YCrCb);
	cvNot(img_inv_copy, img_inv_copy);
	cvCvtColor(img_inv_copy, img_inv_copy, CV_YCrCb2BGR);

	/* mark the holes black on the image, for the correlation with the white
	ball to	be small there */
	paintHolesBlack(img_inv_copy);

	/* set the ROI to be only the table itself (actually a bounding rect which
	is a little bigger */
	cvSetImageROI(img_copy, tableBordersBoundingRect());
	cvSetImageROI(img_inv_copy, tableBordersBoundingRect());

	/* search for the balls one by one */
	for(int i=0; i<n_balls; i++) {
		// find the template
		vector<CvPoint> p;

		if(BALLS_FIND_DEBUG) {
			cout<<"Finding "<<i<<endl;
		}

		// actually find the template
		if(i == 0) { // pink or white?
			p = findPinkOrWhite(img_copy, ball_templates[0], ball_templates[1], ball_thds[0]);
		} else if(ball_inv_templ[i]) {
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

	// release temps
	cvReleaseImage(&img_copy);
	cvReleaseImage(&img_inv_copy);
}


vector<CvPoint> findTemplate(IplImage *img, IplImage *templ, double corr_thd,
							 int max_count, bool custom_norm) {

	/* for the custom_norm -- convert both the image and the template to
	grayscale */
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
	if(!custom_norm) { // regular normalization
		cvMatchTemplate(img, templ, match, CV_TM_CCORR_NORMED);

	} else { // our normalization
		cvMatchTemplate(img, templ, match, CV_TM_CCORR);
		
		// limit the image to non-negative values
		IplImage *limit_mask = cvCreateImage(cvSize(img_s.width - templ_s.width + 1,
			img_s.height - templ_s.height + 1), IPL_DEPTH_8U, 1);
		cvCmpS(match, 0, limit_mask, CV_CMP_LT);
		cvSet(match, cvScalar(0), limit_mask);
		cvReleaseImage(&limit_mask);

		// shift and scale to [0,1]
		double tmp_max, tmp_min;
		cvMinMaxLoc(match, &tmp_min, &tmp_max, 0, 0, 0);
		cvAddS(match, cvScalar(-1*tmp_min), match);
		cvConvertScale(match, match, 1/(tmp_max-tmp_min), 0);
	}

	if(BALLS_FIND_DEBUG) {
		cvNamedWindow("Original Image", CV_WINDOW_AUTOSIZE);
		cvShowImage("Original Image", img);
	}

	// the output vector
	vector<CvPoint> res;

	// find the maximas of the correlation image
	for(int i = 0; i < max_count; i++) {
		// find the max point and value
		CvPoint max_p;
		double max_val;
		cvMinMaxLoc(match, 0, &max_val, 0, &max_p, 0);

		if(BALLS_FIND_DEBUG) {
			cvNamedWindow("Template", CV_WINDOW_AUTOSIZE);
			cvShowImage("Template", templ);
			
			// paint the corr. and the circle on match_draw
			IplImage *match_draw = createBlankCopy(match, 3);
			cvCvtColor(match, match_draw, CV_GRAY2BGR);
			cvCircle(match_draw, max_p, 1, cvScalar(0xff, 0), 2);
			cout<<"corr: "<<max_val<<" vs "<<corr_thd<<endl;
			cvNamedWindow("Template matching", CV_WINDOW_AUTOSIZE);
			cvShowImage("Template matching", match_draw);
			cvWaitKey(0);
			cvDestroyWindow("Template matching");
			cvDestroyWindow("Template");
			cvDestroyWindow("Original Image");

			cvReleaseImage(&match_draw);

			
		}

		if(max_val < corr_thd) {
			// if we're below the correlation threshold, die
			i = max_count;
		} else {
			if(custom_norm) {
				/* if it's our normalization then we check that the value (1)
				is enough above the mean of the image */
				double mean = cvAvg(match).val[0];
				
				if(BALLS_FIND_DEBUG) {
					cout<<"1-mean = "<<max_val-mean<<endl;
				}

				if(max_val-mean < corr_thd) { // no black ball. not enough variance
					i = max_count;
					continue;
				}
			}
			
			/* mark the found ball as black on the image, in order to look for
			the next maxima */
			cvCircle(match, max_p, 1, cvScalar(0), BALL_DIAMETER);

			/* fix the position of the found ball to be the middle of the
			template */
			max_p.x = max_p.x + (templ->width)/2;
			max_p.y = max_p.y + (templ->height)/2;
			
			// add to the output vector
			res.push_back(max_p);
		}
	}

	// release temps
	cvReleaseImage(&match);
	if(custom_norm) { // release the grayscale images
		cvReleaseImage(&img);
		cvReleaseImage(&templ);
	}

	return res;
}

vector<CvPoint> findPinkOrWhite(IplImage *img, IplImage *templ_p,
								 IplImage *templ_w, double corr_thd) {

	CvSize img_s = cvGetSize(img);
	CvSize templ_w_s = cvGetSize(templ_w);
	CvSize templ_p_s = cvGetSize(templ_p);

	// create an image the size the correlation function return
	IplImage *match_w = cvCreateImage(cvSize(img_s.width - templ_w_s.width + 1,
		img_s.height - templ_w_s.height + 1), IPL_DEPTH_32F, 1);
	IplImage *match_p = cvCreateImage(cvSize(img_s.width - templ_p_s.width + 1,
		img_s.height - templ_p_s.height + 1), IPL_DEPTH_32F, 1);

	cvMatchTemplate(img, templ_w, match_w, CV_TM_CCORR_NORMED);
	cvMatchTemplate(img, templ_p, match_p, CV_TM_CCORR_NORMED);

	// the output vector
	vector<CvPoint> res;

	// find the max point and value
	CvPoint max_p_w;
	double max_val_w;
	CvPoint max_p_p;
	double max_val_p;
	cvMinMaxLoc(match_w, 0, &max_val_w, 0, &max_p_w, 0);
	cvMinMaxLoc(match_p, 0, &max_val_p, 0, &max_p_p, 0);

	if(max_val_p >= corr_thd) {
		// check if it's really the pink or it's actually white
		if(max_val_w >= max_val_p && dist(max_p_p, max_p_w) <= BALL_DIAMETER) {
			// not really found! it's the white.
		} else {
			/* fix the position of the found ball to be the middle of the
			template */
			max_p_p.x = max_p_p.x + (templ_p->width)/2;
			max_p_p.y = max_p_p.y + (templ_p->height)/2;
			
			// add to the output vector
			res.push_back(max_p_p);
		}
	}

	// release temps
	cvReleaseImage(&match_p);
	cvReleaseImage(&match_w);

	return res;
}

IplImage **ballTemplates() {
	static IplImage* ball_templates[8];

	static bool once = true;
	if(once) {
		int i;
		char **ball_filenames = ballFilenames();
		for(i=0; i<NUM_BALLS; i++) {
			ball_templates[i] = cvLoadImage(ball_filenames[i]);
		}
		once = false;
	}

	return ball_templates;
}

char **ballFilenames() {
	static char *ball_filenames[8] = {"pink-templ.jpg", "white-templ.jpg",
		"red-templ.jpg", "yellow-templ.jpg", "green-templ.jpg",
		"brown-templ.jpg", "blue-templ.jpg",  "black-templ.jpg"};

	return ball_filenames;
}

CvScalar* ballInvColors() {
	static CvScalar ball_inv_colors[8] = {cvScalar(52, 63, 0), cvScalar(0,0,0),
		cvScalar(255, 255, 0), cvScalar(255, 0, 0), cvScalar(255, 0, 255),
		cvScalar(255, 180, 105), cvScalar(0, 255, 255), 
		cvScalar(255, 255, 255)};

	return ball_inv_colors;
}

int* ballMaxCounts() {
	static int ball_counts[8] = {1, 1, 15, 1, 1, 1, 1, 1};
	return ball_counts;
}

double* ballCorrThds() {
	//static double ball_thd[8] = {0.9, 0.9, 0.8, 0.9, 0.7, 0.7, 0.85, 0.22};
	static double ball_thd[8];

	static bool once = true;
	if(once) {
		// read from file only once
		CvMat* thds = (CvMat*)cvLoad("corr-thd.xml");

		int i;
		for(i=0; i<NUM_BALLS; i++) {
			ball_thd[i] = CV_MAT_ELEM(*thds, float, 0, i);
		}

		once = false;
	}

	return ball_thd;
}

bool* ballInverseTempls() {
	static bool ball_inv_templ[8] = {false, false, false, false, false, false, false,
		true};
	return ball_inv_templ;
}

char* ballTCPPrefixes(){
	static char ball_tcp_prefix[8] = {'p', 'w', 'r', 'y', 'g', 'o', 'l', 'b'};
	return ball_tcp_prefix;
}

void findBallCorrelations(IplImage* img, IplImage* ball_templates[],
						  bool ball_inv_templ[], double ball_corr[],
						  int n_balls) {

	double thd_diffs[] = BALL_CORR_THD;
	  
	/* this is a copy of the image we mark find balls on (in order for the
	correlation there with any other template to be small) */
	IplImage *img_copy = cvCloneImage(img);
	/* this is exactly the same only with the inverted image, to search for
	inverted templates on */
	IplImage *img_inv_copy = createBlankCopy(img_copy);
	cvCvtColor(img_copy, img_inv_copy, CV_BGR2YCrCb);
	cvNot(img_inv_copy, img_inv_copy);
	cvCvtColor(img_inv_copy, img_inv_copy, CV_YCrCb2BGR);

	/* mark the holes black on the image, for the correlation with the white
	ball to	be small there */
	paintHolesBlack(img_inv_copy);

	/* set the ROI to be only the table itself (actually a bounding rect which
	is a little bigger */
	cvSetImageROI(img_copy, tableBordersBoundingRect());
	cvSetImageROI(img_inv_copy, tableBordersBoundingRect());

	/* search for the balls one by one */
	for(int i=0; i<n_balls; i++) {
		// find the template and correlation
		CvPoint p;
		double corr;
		if(ball_inv_templ[i]) {
			p = findTemplateCorrelation(img_inv_copy, ball_templates[i],
				true, &corr);
		} else {
			p = findTemplateCorrelation(img_copy, ball_templates[i],
				false, &corr);
		}

		// paint the found ball black
		cvCircle(img_copy, p, 1, cvScalar(0), BALL_DIAMETER*3/4);
		cvCircle(img_inv_copy, p, 1, cvScalar(0), BALL_DIAMETER*3/4);

		cout<<"corr["<<i<<"]:"<<corr<<",  ths:<<";
		// add the thd. to the correlation
		corr -= thd_diffs[i];
		cout<<corr<<endl;

		// set on the results array
		ball_corr[i] = corr;
	}

	// release temps
	cvReleaseImage(&img_copy);
	cvReleaseImage(&img_inv_copy);
}

CvPoint findTemplateCorrelation(IplImage *img, IplImage *templ, bool custom_norm,
							 double *corr) {

	/* for the custom_norm -- convert both the image and the template to
	grayscale */
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
	if(!custom_norm) { // regular normalization
		cvMatchTemplate(img, templ, match, CV_TM_CCORR_NORMED);

	} else { // our normalization
		cvMatchTemplate(img, templ, match, CV_TM_CCORR);
		
		// limit the image to non-negative values
		IplImage *limit_mask = cvCreateImage(cvSize(img_s.width - templ_s.width + 1,
			img_s.height - templ_s.height + 1), IPL_DEPTH_8U, 1);
		cvCmpS(match, 0, limit_mask, CV_CMP_LT);
		cvSet(match, cvScalar(0), limit_mask);
		cvReleaseImage(&limit_mask);

		// shift and scale to [0,1]
		double tmp_max, tmp_min;
		cvMinMaxLoc(match, &tmp_min, &tmp_max, 0, 0, 0);
		cvAddS(match, cvScalar(-1*tmp_min), match);
		cvConvertScale(match, match, 1/(tmp_max-tmp_min), 0);
	}

	// find the maximum of the correlation image
	CvPoint max_p;
	double max_val;
	cvMinMaxLoc(match, 0, &max_val, 0, &max_p, 0);

	// set the found correlation (or equiv.) in the output variable
	if(custom_norm) {
		double mean = cvAvg(match).val[0];

		*corr = max_val-mean;
		cout<<*corr<<endl;
	} else {
		*corr = max_val;
	}

	/* fix the position of the found ball to be the middle of the
	template */
	max_p.x = max_p.x + (templ->width)/2;
	max_p.y = max_p.y + (templ->height)/2;

	// release temps
	cvReleaseImage(&match);
	if(custom_norm) { // release the grayscale images
		cvReleaseImage(&img);
		cvReleaseImage(&templ);
	}

	// return the found position
	return max_p;
}