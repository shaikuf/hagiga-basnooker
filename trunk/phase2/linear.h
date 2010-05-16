#ifndef _LINEAR_H
#define _LINEAR_H

#include <vector>
#include <cv.h>

using namespace std;

/* this finds points which reside on a line with some R^2 coefficient, and the
line parameters */
vector<CvPoint> findPointsOnLine(const vector<CvPoint2D32f> &points,
								 double min_coeff, double *line_m,
								 double *line_n, CvPoint *line_cm);

/* this finds the line parameters and R^2 of a list of points */
void linearRegression(vector<CvPoint2D32f> points, double *m_a, double *m_b,
					  double *m_coeff);

/* this finds the distance between a point (x,y) and a line: y = m_a + m_b*x */
double distFromLine(double x, double y, double m_a, double m_b);

#endif