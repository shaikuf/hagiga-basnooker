#include "linear.h"
#include <cv.h>
#include <iostream>
#include <vector>

using namespace std;

/* this finds points which reside on a line with some R^2 coefficient, and the
line parameters */
vector<CvPoint> findPointsOnLine(const vector<CvPoint2D32f> &points,
								 double min_coeff, double *line_m,
								 double *line_n, CvPoint *line_cm) {
	vector<CvPoint2D32f> d_points(points);

	double m_a, m_b, m_coeff = 0;

	while(m_coeff < min_coeff && d_points.size() >= 2) {
		linearRegression(d_points, m_a, m_b, m_coeff);

		// delete the furthest point
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

		d_points.erase(max_iter);
	}

	if(d_points.size() < 2) {
		line_cm->x = -1;
		line_cm->y = -1;
		vector<CvPoint> empty;
		return empty;
	}

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

	return res_points;
}

/* this finds the distance between a point (x,y) and a line: y = m_a + m_b*x */
double distFromLine(double x, double y, double m_a, double m_b) {
	return fabs(-m_b*x + y -m_a)/sqrt(m_b*m_b + 1);
}

/* this finds the line parameters and R^2 of a list of points */
void linearRegression(vector<CvPoint2D32f> points, double &m_a, double &m_b,
					  double &m_coeff) {
	int n = points.size();

	// calculate means
	double mean_x = 0, mean_y = 0;
	for(int i=0; i<n; i++) {
		mean_x += points[i].x;
		mean_y += points[i].y;
	}
	mean_x /= n;
	mean_y /= n;

	// calculate std and sample correlation coefficient
	double std_x = 0, std_y = 0;
	double r_xy = 0;
	for(int i=0; i<n; i++) {
		double diff_x = points[i].x-mean_x, diff_y = points[i].y-mean_y;
		std_x += diff_x*diff_x;
		std_y += diff_y*diff_y;
		r_xy += diff_x*diff_y;
	}
	std_x = sqrt(std_x/n);
	std_y = sqrt(std_y/n);
	r_xy /= std_x*std_y*(n-1);

	// calculate fit coefficients
	m_b = r_xy*std_y/std_x;
	m_a = mean_y - m_b*mean_x;
	
	// calculate r^2
	m_coeff = r_xy*r_xy;
}