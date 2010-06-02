#ifndef _LINEAR_H
#define _LINEAR_H

#include <vector>
#include <cv.h>

using namespace std;

/*	Finds a subset of points which reside on a line with some minimal R^2
	coefficient, and the line parameters.
	Gets:
		(const vector<CvPoint2D32f>&)points		the given set of points
		(double)min_coeff	the minimal R^2 coefficient
		(double*)line_m		a return variable for the slope of the found line
		(double*)line_n		a return variable for the intersect of the found
							line
		(CvPoint*)line_cm	a return variable for the center of mass of the
							found line
	Returns:
		(vector<CvPoint>)	the vector of the subset of points */
vector<CvPoint> findPointsOnLine(const vector<CvPoint2D32f> &points,
								 double min_coeff, double *line_m,
								 double *line_n, CvPoint *line_cm,
								 bool fix_first = false);
vector<CvPoint> findPointsOnLineWith(CvPoint fixed_pt,
									const vector<CvPoint2D32f> &points,
									double min_coeff, double *line_m,
									double *line_n, CvPoint *line_cm);
vector<CvPoint> findPointsOnLineByGrowing(const vector<CvPoint2D32f> &points,
										  double min_coeff, double *line_m,
										  double *line_n, CvPoint *line_cm);

/*	Finds the line parameters and R^2 coefficient for a simple linear
	regression of the given vector of points.
	Gets:
		(vector<CvPoint2D32f>)points	the given set of points
		(double*)m_a	a return variable for the intersect of the found line
						(alpha)
		(double*)m_b	a return variable for the slope of the found line
						(beta)
		(double*)m_coeff	a return variable for the appropriate R^2 */
void linearRegression(vector<CvPoint2D32f> points, double *m_a, double *m_b,
					  double *m_coeff);
void linearRegression(vector<CvPoint> points, double *m_a, double *m_b,
					  double *m_coeff);

/*	Finds the distance between a point (x,y) and a line: y = m_a + m_b*x
	Gets:
		(double)x	the x coordinate of the point
		(double)y	the y coordinate of the point
		(double)m_a	the intersect of the line
		(double)m_b	the slope of the line
	Returns:
		(double)	the distance between the point and the line */
double distFromLine(double x, double y, double m_a, double m_b);

#endif