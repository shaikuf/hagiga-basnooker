#include "linear.h"
#include "cue.h"
#include <cv.h>
#include <iostream>
#include <vector>

#include "params.h"

using namespace std;

vector<CvPoint> findPointsOnLine(const vector<CvPoint2D32f> &points,
								 double min_coeff, double *line_m,
								 double *line_n, CvPoint *line_cm) {

	// not enough blobs
	if(points.size() < CUE_MIN_BLOBS) {
		line_cm->x = -1;
		line_cm->y = -1;
		vector<CvPoint> empty;
		return empty;
	}

	// the vector of final points
	vector<CvPoint2D32f> d_points(points);

	// regression parameters
	double m_a, m_b, m_coeff = 0;

	// find the linear regression
	linearRegression(d_points, &m_a, &m_b, &m_coeff);

	// while it's not good enough and we have enough points:
	while(m_coeff < min_coeff && d_points.size() >= CUE_MIN_BLOBS) {
		// find the furthest point
		vector<CvPoint2D32f>::iterator max_iter = d_points.begin();
		double max_dist = distFromLine(max_iter->x, max_iter->y, m_a, m_b);
		vector<CvPoint2D32f>::iterator iter = max_iter+1;
		while(iter != d_points.end()) {
			double new_dist;
			if((new_dist = distFromLine(iter->x, iter->y, m_a, m_b)) > max_dist) {
				max_dist = new_dist;
				max_iter = iter;
			}
			iter++;
		}

		// delete it
		d_points.erase(max_iter);

		// find the new linear regression
		linearRegression(d_points, &m_a, &m_b, &m_coeff);
	}

	// if we didn't found a line good enough
	if(d_points.size() < CUE_MIN_BLOBS) {
		line_cm->x = -1;
		line_cm->y = -1;
		vector<CvPoint> empty;
		return empty;
	}

	// find the center of mass and create the output vector
	double cm_x = 0, cm_y = 0;

	vector<CvPoint> res_points;
	for(unsigned int i=0; i<d_points.size(); i++) {
		cm_x += d_points[i].x;
		cm_y += d_points[i].y;

		res_points.push_back(cvPoint((int)d_points[i].x, (int)d_points[i].y));
	}

	line_cm->x = (int)(cm_x/d_points.size());
	line_cm->y = (int)(cm_y/d_points.size());

	*line_m = m_b;
	*line_n = m_a;

	if(CUE_FIND_DEBUG) {
		cout<<"exiting with "<<m_coeff<<" from "<<res_points.size()<<" points"<<endl;
	}
	return res_points;
}

double distFromLine(double x, double y, double m_a, double m_b) {
	return fabs(-m_b*x + y -m_a)/sqrt(m_b*m_b + 1);
}

void linearRegression(vector<CvPoint2D32f> points, double *m_a, double *m_b,
					  double *m_coeff) {
	int n = points.size();

	// calculate means
	double mean_x = 0, mean_y = 0;
	for(int i=0; i<n; i++) {
		mean_x += points[i].x;
		mean_y += points[i].y;
	}
	mean_x /= n;
	mean_y /= n;

	// calculate sums of square differences
	double std_x = 0, std_y = 0;
	double r_xy = 0;
	for(int i=0; i<n; i++) {
		double diff_x = points[i].x-mean_x, diff_y = points[i].y-mean_y;
		std_x += diff_x*diff_x;
		std_y += diff_y*diff_y;
		r_xy += diff_x*diff_y;
	}

	// calculate fit coefficients
	*m_b = r_xy / std_x;
	*m_a = mean_y - (*m_b)*mean_x;
	
	// calculate r^2
	double s_xy = 0, s_x = 0, s_y = 0, s_x2 = 0, s_y2 = 0;
	for(int i=0; i<n; i++) {
		s_xy += points[i].x*points[i].y;
		s_x += points[i].x;
		s_y += points[i].y;
		s_x2 += points[i].x*points[i].x;
		s_y2 += points[i].y*points[i].y;
	}

	double r = (n*s_xy - s_x*s_y)/(sqrt(n*s_x2 - s_x*s_x)*sqrt(n*s_y2 - s_y*s_y));
	*m_coeff = r*r;
}
