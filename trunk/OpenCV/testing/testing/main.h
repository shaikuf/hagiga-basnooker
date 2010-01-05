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
					float *radius);

/* Mouse callback wrapper for findBallAround. param is the img to work on */
void findBallAroundMouse(int event, int x, int y, int flags, void *param);

/* Finds and marks the cue stick on the image */
void markStickOnImage(IplImage *src);

#endif _MAIN_H