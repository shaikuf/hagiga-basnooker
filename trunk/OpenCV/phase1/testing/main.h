#ifndef _MAIN_H
#define _MAIN_H

#define PI 3.14159265

/* Get the background from multiple background files and avergage them */
IplImage* getBackground();

/* Create a new IplImage, the same size of src. If channels is -1, it's also
the same number of channels. The same for depth. */
IplImage *createBlankCopy(IplImage *src, int channels = -1, int depth = -1);

/* Normalize the image for debugging, so it could fit on the screen. */
void normalize(IplImage* &img, CvRect crop, CvSize size);

/* Set the channels a,b,c of src to:
a/(a+b+c), b/(a+b+c), c/(a+b+c).
This essentially normalizes the brightness and contrast, against global
lighting changes.*/
void equalize(IplImage *src, IplImage *dst);

/* Find circles using Hough Transform, and draw them on the returned image,
overlayed on the original image. */
IplImage *findCircles(IplImage *src, int canny, int thresh, int min_dist,
					  int min_radius, int max_radius);

/* Compute a mask according to a threshold on the difference from a certain
color (thresh may be different for each channel) */
void colorThresh(IplImage *src_image, IplImage *dst_mask, CvScalar color, 
                                 CvScalar thresh);

/* Finds the center and radius of the maximal circle containing the point
given, and not containing any non-zero point in the given image. */
void findMaxContainedCircle(IplImage *bw, CvPoint inside,
							CvPoint2D32f *f_center, float *f_radius,
							int thresh);

/* This tries to find the center and radius of the ball which contains
the given point, in the image. */
void findBallAround(IplImage *src, CvPoint inside, CvPoint2D32f *center,
					float *radius, bool debug = true);

/* Mouse callback wrapper for findBallAround. param is the img to work on */
void findBallAroundMouse(int event, int x, int y, int flags, void *param);

/* Shows an image and forwared clicks on it to the given function until ESC is
met */
void genericMouseWrapper(IplImage *img, void (*func)(int, int, int, int, void *));

/* Mouse callback wrapper that prints the color of the clicked point. param
is the img to work on */
void findColorAroundMouse(int event, int x, int y, int flags, void *param);

/* Mouse callback wrapper that prints the position of the clicked point.
param is the img to work on */
void findPosAroundMouse(int event, int x, int y, int flags, void *param);

/* Finds the point which best matches the template */
void findTemplate(IplImage *img, IplImage *templ, CvPoint *p,
				  bool debug = true);

/* This uses findBall() and marks the results on the image */
void markBall(IplImage *img, IplImage *templ, bool circle = true);

/* This finds the ball matching the given template on the image */
void findBall(IplImage *img, IplImage *templ, CvPoint2D32f *center,
			  float *radius, bool debug = true);

/* This uses findCue() and marks the results on the image (around the white
ball) */
void markCue(IplImage *src, CvPoint2D32f white_center, float white_radius);

/* This finds the center-of-mass and slope of the cue in the given image */
void findCue(IplImage *src, double *cue_m, CvPoint *cue_cm);

/* This filters lines which are completely outside tableBorders() */
void filterLinesOnTable(CvSeq *lines);

/* This filters lines with very similiar slope and position */
void filterSimiliarLines(CvSeq *lines, double m_thresh, double n_thresh);

/* This filters lines with not-near-enough-to-template histogram around them */
void filterLinesByHistogram(IplImage *src, CvSeq *lines, int nbins, int width,
							 double corr_thresh);

/* This draws a visualization image of the given histogram, on the window
with the given name */
void drawHistogram(char *name, CvHistogram *hist, int nbins);

/* This returns the histogram taken around a line in the given image */
CvHistogram *hueHistFromLine(IplImage *img, CvPoint p1, CvPoint p2, int width,
							 int nbins);

/* This returns the histogram taken around the cue in a specific sample
image */
CvHistogram *cueHistogram(int width, int nbins);

/* This returns a sequence of CvPoints specifying the contour of the
board borders */
CvSeq *tableBorders(CvMemStorage *mem);

/* This finds the mean slope for the given sequence of lines.
Actually it returns the average slope for the two extreme slopes.*/
void meanLine(CvSeq *lines, double *dst_m);

/* This finds the mean center-of-mass for the given sequence of lines */
void meanCM(CvSeq *lines, CvPoint *dst_cm);

/* This resizes images before showing them (and possibly saves to a file)
so that they would fit on the screen */
void cvShowImageWrapper(const char *name, IplImage *image);

#endif _MAIN_H