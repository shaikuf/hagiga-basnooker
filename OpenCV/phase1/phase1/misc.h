#ifndef _MISC_H
#define _MISC_H

#include <cv.h>

#define PI 3.14159265

extern int downscale_factor;
extern bool save_images;

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

/* Compute a mask according to a threshold on the difference from a certain
color (thresh may be different for each channel) */
void colorThresh(IplImage *src_image, IplImage *dst_mask, CvScalar color, 
                                 CvScalar thresh);

/* Finds the point which best matches the template */
void findTemplate(IplImage *img, IplImage *templ, CvPoint *p,
				  bool debug = true);

/* This draws a visualization image of the given histogram, on the window
with the given name */
void drawHistogram(char *name, CvHistogram *hist, int nbins);

/* This returns a sequence of CvPoints specifying the contour of the
board borders */
CvSeq *tableBorders(CvMemStorage *mem);

/* This resizes images before showing them (and possibly saves to a file)
so that they would fit on the screen */
void cvShowImageWrapper(const char *name, IplImage *image);

/* This finds and draws the borders of the table */
void drawBorders(IplImage *dst, int width);

#endif