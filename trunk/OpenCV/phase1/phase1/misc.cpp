#include <cv.h>
#include <highgui.h>
#include <limits>
#include <cmath>
#include "misc.h"

using namespace std;

/* Finds the point which best matches the template */
void findTemplate(IplImage *img, IplImage *templ, CvPoint *p, bool debug) {
	CvSize img_s = cvGetSize(img);
	CvSize templ_s = cvGetSize(templ);
	IplImage *match = cvCreateImage(cvSize(img_s.width - templ_s.width + 1,
		img_s.height - templ_s.height + 1), IPL_DEPTH_32F, 1);

	cvMatchTemplate(img, templ, match, CV_TM_CCORR_NORMED);

	CvPoint min_p;
	cvMinMaxLoc(match, 0, 0, 0, &min_p/*, 0*/, 0);

	if(debug) {
		cvNamedWindow("Template matching", CV_WINDOW_AUTOSIZE);
		IplImage *match_draw = createBlankCopy(match, 3);
		cvMerge(match, match, match, 0, match_draw);
		cvCircle(match_draw, min_p, 1, cvScalar(0xff, 0), 2);
		cvShowImageWrapper("Template matching", match_draw);
		cvReleaseImage(&match_draw);
		cvWaitKey(0);
		cvDestroyWindow("Template matching");
	}

	cvReleaseImage(&match);

	p->x = min_p.x + (templ->width)/2;
	p->y = min_p.y + (templ->height)/2;
}

/* This returns a sequence of CvPoints specifying the contour of the
board borders */
CvSeq *tableBorders(CvMemStorage *mem) {
	char filename[100];
	_snprintf_s(filename, 100, "borders-%d.xml", 0);
	CvSeq* borders = (CvSeq*)cvLoad(filename, mem);

	return borders;
}

/* This finds and draws the borders of the table */
void drawBorders(IplImage *dst, int width) {
	CvMemStorage *mem = cvCreateMemStorage();
	CvSeq *borders = tableBorders(mem);

	for(int i = 0; i < borders->total; i++) {
		CvPoint p1 = *(CvPoint*)cvGetSeqElem(borders, i);
		CvPoint p2 = *(CvPoint*)cvGetSeqElem(borders, (i+1)%borders->total);

		cvLine(dst, p1, p2, cvScalar(0, 255, 0), width);
	}

	cvReleaseMemStorage(&mem);
}

/* This draws a visualization image of the given histogram, on the window
with the given name */
void drawHistogram(char *name, CvHistogram *hist, int nbins) {
	//create an image to hold the histogram
	IplImage* histImage = cvCreateImage(cvSize(320,200), 8, 1);

	//grab the min and max values and their indeces
	float min_value, max_value;
	cvGetMinMaxHistValue( hist, &min_value, &max_value);
	//scale the bin values so that they will fit in the image representation
	cvScale( hist->bins, hist->bins, ((double)histImage->height)/max_value, 0 );

	//set all histogram values to 255
	cvSet( histImage, cvScalarAll(255), 0 );
	//create a factor for scaling along the width
	int bin_w = cvRound((double)histImage->width/nbins);

	for(int k = 0; k < nbins; k++ ) {
		//draw the histogram data onto the histogram image
		cvRectangle( histImage, cvPoint(k*bin_w, histImage->height),
		   cvPoint((k+1)*bin_w, 
		   histImage->height - cvRound(cvGetReal1D(hist->bins,k))),
		   cvScalarAll(0), -1, 8, 0 );
		//get the value at the current histogram bucket
		float* bins = cvGetHistValue_1D(hist,k);
	}

	cvShowImageWrapper(name, histImage);

	cvReleaseImage(&histImage);
}

/* Create a new IplImage, the same size of src. If channels is -1, it's also
the same number of channels. The same for depth. */
IplImage *createBlankCopy(IplImage *src, int channels, int depth) {
	if(depth == -1)
		depth = src->depth;
	if(channels == -1)
		channels = src->nChannels;

	return cvCreateImage(cvGetSize(src), depth, channels);
}

/* This resizes images before showing them (and possibly saves to a file)
so that they would fit on the screen */
void cvShowImageWrapper(const char *name, IplImage *image) {
	if(save_images) {
		char filename[1024];
		static int num = 0;
		_snprintf_s(filename, 1024, "C:\\Projecton\\Test\\Results\\%d.jpg", num);
		num++;
		IplImage *copy = createBlankCopy(image, image->nChannels, IPL_DEPTH_8U);
		cvConvertImage(image, copy);
		cvSaveImage(filename, copy);
		cvReleaseImage(&copy);
	}

	CvRect crop;
	CvSize size;


	if(cvGetSize(image).width == 1600 && cvGetSize(image).height == 1200) {
		crop = cvRect(0, 0, 1600, 850);
		size = cvSize(800, 425);
	} else if(cvGetSize(image).width == 1553 && cvGetSize(image).height == 1155) {
		crop = cvRect(0, 0, 1550, 1150);
		size = cvSize(775, 575);
	} else if(cvGetSize(image).width == 1553 && cvGetSize(image).height == 805) {
		crop = cvRect(0, 0, 1550, 800);
		size = cvSize(775, 400);
	} else {
		cvShowImage(name, image);
		return;
	}

	IplImage *temp = cvCloneImage(image);
	normalize(temp, crop, size);
	cvShowImage(name, temp);
	cvReleaseImage(&temp);
}

/* Normalize the image for debugging, so it could fit on the screen. */
void normalize(IplImage* &img, CvRect crop, CvSize size) {
	cvSetImageROI(img, crop);
	IplImage *img_new = cvCreateImage(size, img->depth, img->nChannels);
	cvPyrDown(img, img_new);
	cvReleaseImage(&img);
	img = img_new;
}

