#ifndef _BALLS_H
#define _BALLS_H

#include <cv.h>
#include <vector>

using namespace std;

/*	Fix an absolute position on the image, to a position relative to the table.
	Gets:
		(CvPoint)center		a point in the image
	Returns:
		(CvPoint)			a point on the table with 0<=X,Y<=1 */
CvPoint2D32f fixPosition(CvPoint center);

/*	This finds all the balls at once
	Gets:
		(IplImage*) img				the image we search on
		(IplImage**) ball_templates	the templates we search for
		(int*) ball_counts			the max number of balls on the table for
									each template
		(bool*) ball_inv_templ		whether or not each template is an inverse
		(double*) ball_thds			the threshold values for the correlation
									for each template
		(vector<CvPoint>*) ball_centers
									an output array for the found position
									vectors of each template
		(int) n_balls				the number of templates (size of arrays) */
void findBalls(IplImage *img, IplImage *ball_templates[],
						 int ball_counts[], bool ball_inv_templ[],
						 double ball_thds[], vector<CvPoint> ball_centers[],
						 int n_balls);

/* Find the points which best matches a template
	Gets:
		(IplImage*) img		the image to search on
		(IplImage*) templ	the template to search for
		(double) corr_thd	the minimal correlation to say that it has been
							found
		(int) max_count		the maximal number of found points
		(bool) custom_norm	whether or not to use our own normalization on
							the correlation image
	Returns:
		(vector<CvPoint>)	the vector of found centers */
vector<CvPoint> findTemplate(IplImage *img, IplImage *templ, double corr_thd,
							 int max_count, bool custom_norm = false);

// ...
vector<CvPoint> findPinkOrWhite(IplImage *img, IplImage *templ_p,
								 IplImage *templ_w, double corr_thd);

/*	Returns an array of the ball templates. */
IplImage **ballTemplates();
/*	Returns an array of the ball templates' filenames. */
char **ballFilenames();
/*	Returns an array of the inverses of the ball colors. */
CvScalar* ballInvColors();
/*	Returns an array of the maxmimum number of balls of these types on
	the table. */
int* ballMaxCounts();
/*	Returns an array of the minimal correlation for each ball type to say
	that it was found. */
double* ballCorrThds();
/*	Returns an array stating whether each ball has an "inverse" template. */
bool* ballInverseTempls();
/*	Returns an array of the prefixes we use when sending each ball position
	over TCP to the GUI application. */
char* ballTCPPrefixes();

/*	Computes an array of the correlations when finding each ball template.
	Gets:
		(IplImage*) img				the image we search on
		(IplImage**) ball_templates	the templates we search for
		(bool*) ball_inv_templ		whether or not each template is an inverse
		(double*) ball_corr			an output array for the found correlations
		(int) n_balls				the number of templates (size of arrays) */
void findBallCorrelations(IplImage* img, IplImage* ball_templates[],
						  bool ball_inv_templ[], double ball_corr[],
						  int n_balls);

/*	Finds the maximum correlation (value and coordinates) for a template in an
	image.
	Gets:
		(IplImage*) img			the image we search on
		(IplImage*) templ		the template we search for
		(bool) custom_norm		whether or not to use our own normalization on
								the correlation image
		(double*) corr			an output variable for the found correlation
	Returns:
		(CvPoint)				the coordinates of the max correlation point */
CvPoint findTemplateCorrelation(IplImage *img, IplImage *templ, bool custom_norm,
							 double *corr);

#endif