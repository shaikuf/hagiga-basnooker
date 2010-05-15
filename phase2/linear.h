#ifndef _LINEAR_H
#define _LINEAR_H

#include <vector>
#include <cv.h>

using namespace std;


vector<CvPoint> findPointsOnLine(const vector<CvPoint2D32f> &points,
								 double min_coeff, double *line_m,
								 double *line_n, CvPoint *line_cm);
void linearRegression(vector<CvPoint2D32f> points, double &m_a, double &m_b, double &m_coeff);
double distFromLine(double x, double y, double m_a, double m_b);

#endif