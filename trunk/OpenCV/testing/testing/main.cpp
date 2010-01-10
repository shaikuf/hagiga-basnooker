#include <highgui.h>
#include <cv.h>
#include <math.h>
#include <iostream>
#include <time.h>
#include <limits.h>
#include "camera_interface.h"
#include "main.h"

using namespace std;

int downscale_factor = 1;
bool save_images = false;

int main(int argc, char* argv[])
{
	IplImage *img = cvLoadImage("C:\\Projecton\\Test\\Testing\\"
		"Picture 33.jpg");  //25 30 (32 is good but the way is not) 35 39

	IplImage *templ = cvLoadImage("C:\\Projecton\\Test\\Testing\\"
		"WhiteBall.jpg");
	//normalize(templ, cvRect(0, 0, 48, 46), cvSize(24, 23));

	CvPoint2D32f p;
	float radius;
	findBall(img, templ, &p, &radius, false);
	markBall(img, templ, false);
	markCue(img, p, radius);

	//genericMouseWrapper(img_a, findColorAroundMouse);
		// cue hue: 21 +- 1
		// white hue: 27-30
	//genericMouseWrapper(img, findBallAroundMouse);
	//genericMouseWrapper(img, findPosAroundMouse);

	cvNamedWindow("Final", CV_WINDOW_AUTOSIZE);
	cvShowImageWrapper("Final", img);
	cvWaitKey(0);
	cvDestroyWindow("Final");

	cvReleaseImage(&img);
	cvReleaseImage(&templ);

	return 0;
}

/* This resizes images before showing them (and possibly saves to a file)
so that they would fit on the screen */
void cvShowImageWrapper(const char *name, IplImage *image) {
	if(save_images) {
		char filename[1024];
		static int num = 0;
		sprintf(filename, "C:\\Projecton\\Test\\Results\\%d.jpg", num);
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

/* Finds the point which best matches the template */
void findTemplate(IplImage *img, IplImage *templ, CvPoint *p, bool debug) {
	CvSize img_s = cvGetSize(img);
	CvSize templ_s = cvGetSize(templ);
	IplImage *match = cvCreateImage(cvSize(img_s.width - templ_s.width + 1,
		img_s.height - templ_s.height + 1), IPL_DEPTH_32F, 1);

	cvMatchTemplate(img, templ, match, CV_TM_SQDIFF_NORMED);

	CvPoint min_p;
	cvMinMaxLoc(match, 0, 0, &min_p, 0, 0);

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

	p->x = min_p.x + (24-1)/2;
	p->y = min_p.y + (23-1)/2;
}

/* This finds the ball matching the given template on the image */
void findBall(IplImage *img, IplImage *templ, CvPoint2D32f *center,
			  float *radius, bool debug) {
	// find the template
	CvPoint p;
	findTemplate(img, templ, &p, debug);

	// find the ball parameters
	findBallAround(img, p, center, radius, debug);
}

/* This uses findBall() and marks the results on the image */
void markBall(IplImage *img, IplImage *templ, bool circle) {
	CvPoint2D32f p_32;
	float radius;

	findBall(img, templ, &p_32, &radius);

	CvPoint center = cvPointFrom32f(p_32);

	// print an overlay image of the found circle
	IplImage *overlay_drawing = createBlankCopy(img, 1);
	cvSet(overlay_drawing, cvScalar(0));

	if(circle) {
		cvCircle(overlay_drawing, cvPoint(cvRound(center.x),
			cvRound(center.y)),	cvRound(radius), cvScalar(0xff), 2);
	}
    int line_len = 5;
	cvLine(overlay_drawing, cvPoint(cvRound(center.x)-line_len,
		cvRound(center.y)), cvPoint(cvRound(center.x)+line_len,
		cvRound(center.y)), cvScalar(0xff), 1);
	cvLine(overlay_drawing, cvPoint(cvRound(center.x),
		cvRound(center.y)-line_len), cvPoint(cvRound(center.x),
		cvRound(center.y)+line_len), cvScalar(0xff), 1);

	/*IplImage *overlay = createBlankCopy(overlay_drawing, 3);
	IplImage *overlay_blank = createBlankCopy(overlay, 1);
	cvSet(overlay_blank, cvScalar(0));
	cvMerge(overlay_blank, overlay_blank, overlay_drawing, 0, overlay);

	IplImage *temp = cvCloneImage(img);

	cvAddWeighted(img, 0.5, overlay, 1, 0, temp);

	cvReleaseImage(&overlay);
	cvReleaseImage(&overlay_drawing);
	cvReleaseImage(&overlay_blank);
	cvReleaseImage(&temp);*/

	cvSet(img, cvScalar(0, 0, 255), overlay_drawing);
}

/* Shows an image and forwared clicks on it to the given function until ESC is
met */
void genericMouseWrapper(IplImage *img, void (*func)(int, int, int, int, void *)) {
	cvNamedWindow("MouseWrapper", CV_WINDOW_AUTOSIZE);
	cvShowImageWrapper("MouseWrapper", img);

	cvSetMouseCallback("MouseWrapper", func, img);
	while(1) {
		if(cvWaitKey(15) == 27)
			break;
	}
	
	cvDestroyWindow("MouseWrapper");
}

/* This returns the histogram taken around a line in the given image */
CvHistogram *hueHistFromLine(IplImage *img, CvPoint p1, CvPoint p2, int width,
							 int nbins) {
	IplImage *hsv = createBlankCopy(img);
	cvCvtColor(img, hsv, CV_BGR2HSV);
	IplImage *h = createBlankCopy(hsv, 1);
	IplImage *s = createBlankCopy(hsv, 1);
	IplImage *v = createBlankCopy(hsv, 1);
	cvSplit(hsv, h, s, v, 0);

	cvReleaseImage(&s);
	cvReleaseImage(&v);
	cvReleaseImage(&hsv);

	float h_range[] = {0., 180.};
	float *ranges[] = {h_range};
	CvHistogram *hist = cvCreateHist(1, &nbins, CV_HIST_ARRAY, (float **)ranges, 1);

	IplImage *mask = createBlankCopy(img, 1);
	cvSet(mask, cvScalar(0));
	cvLine( mask, p1, p2, cvScalar(255), width);

	cvCalcHist(&h, hist, 0, mask);
	cvReleaseImage(&mask);
	cvReleaseImage(&h);

	cvNormalizeHist(hist, 1.);

	return hist;
}

/* This returns the histogram taken around the cue in a specific sample
image */
CvHistogram *cueHistogram(int width, int nbins) {
	CvRect crop = cvRect(0, 0, 1600, 850);
	CvSize size = cvSize(800, 425);

	IplImage *src = cvLoadImage("C:\\Projecton\\Test\\Testing\\"
		"Picture 22.jpg"); 
	normalize(src, crop, size);

	IplImage *gray = createBlankCopy(src, 1);
	cvCvtColor(src, gray, CV_BGR2GRAY);

	IplImage *edge = createBlankCopy(gray);
	cvCanny(gray, edge, 150, 50);

	CvMemStorage *storage = cvCreateMemStorage(0);
	CvSeq *lines = cvHoughLines2(edge, storage, CV_HOUGH_PROBABILISTIC, 0.5, 0.5*CV_PI/180, 80, 50, 50);

	// filter lines not on the board
	//filterLinesOnTable(lines);

	// filter very similar lines
	double m_thresh = 0.05;
	double n_thresh = 10;
	filterSimiliarLines(lines, m_thresh, n_thresh);
	

	CvPoint *line = (CvPoint*)cvGetSeqElem(lines, 1);

	CvHistogram *hist = hueHistFromLine(src, line[0], line[1], width, nbins);
	cvSet1D(hist->bins, 20, cvScalar(0));

	cvReleaseImage(&gray);
	cvReleaseImage(&edge);
	cvReleaseMemStorage(&storage);

	return hist;
}

/* This filters lines with very similiar slope and position */
void filterSimiliarLines(CvSeq *lines, double m_thresh, double n_thresh) {
	for(int i = 0; i < lines->total; i++) {
		CvPoint *line = (CvPoint*)cvGetSeqElem(lines, i);

		double m = ((double)(line[1].y - line[0].y + 0.))/(line[1].x - line[0].x);
		double n = (double)line[0].y - m*line[0].x;

		for(int j = i+1; j < lines->total; j++) {
			CvPoint *line_j = (CvPoint*)cvGetSeqElem(lines, j);
			double m_j = ((double)(line_j[1].y - line_j[0].y + 0.))/
				(line_j[1].x - line_j[0].x);
			double n_j = (double)line_j[0].y - m*line_j[0].x;

			if(fabs(m_j - m) <= m_thresh && fabs(n_j - n) <= n_thresh) {
				cvSeqRemove(lines, j);
				j--;
			}
		}

		//cout<<"("<<line[0].x<<", "<<line[0].y<<") -> (";
		//cout<<line[1].x<<", "<<line[1].y<<")\n";
		//cout<<m<<" "<<n<<endl;
	}
}

/* This returns a sequence of CvPoints specifying the contour of the
board borders */
CvSeq *tableBorders(CvMemStorage *mem) {
	CvSeqWriter writer;
	cvStartWriteSeq(CV_32SC2, sizeof(CvSeq), sizeof(CvPoint), mem, &writer);
	CV_WRITE_SEQ_ELEM(cvPoint(30, 33), writer);
	CV_WRITE_SEQ_ELEM(cvPoint(773, 39), writer);
	CV_WRITE_SEQ_ELEM(cvPoint(799, 62), writer);
	CV_WRITE_SEQ_ELEM(cvPoint(789, 382), writer);
	CV_WRITE_SEQ_ELEM(cvPoint(763, 408), writer);
	CV_WRITE_SEQ_ELEM(cvPoint(34, 393), writer);
	CV_WRITE_SEQ_ELEM(cvPoint(7, 369), writer);
	CV_WRITE_SEQ_ELEM(cvPoint(6, 56), writer);
	CvSeq* borders = cvEndWriteSeq(&writer);

	int border_delta = 13;
	for(int i = 0; i<borders->total; i++) {
		CvPoint *p = (CvPoint*)cvGetSeqElem(borders, i);
		p->x = downscale_factor*2*p->x;
		p->y = downscale_factor*2*p->y;

		if(p->x < downscale_factor*800)
			p->x += downscale_factor*border_delta;
		else
			p->x -= downscale_factor*border_delta;
		if(p->y < downscale_factor*420)
			p->y += downscale_factor*border_delta;
		else
			p->y -= downscale_factor*border_delta;
	}

	return borders;
}

/* This filters lines which are completely outside tableBorders() */
void filterLinesOnTable(CvSeq *lines) {
	// the contour of the board playable area
	CvMemStorage *mem = cvCreateMemStorage();
	CvSeq *borders = tableBorders(mem);

	for(int i = 0; i < lines->total; i++)
    {
		CvPoint *line = (CvPoint*)cvGetSeqElem(lines, i);

		if(!(cvPointPolygonTest(borders, cvPointTo32f(line[0]), false) > 0 ||
			cvPointPolygonTest(borders, cvPointTo32f(line[1]), false) > 0)) {
			cvSeqRemove(lines, i);
			i--;
		}
    }
	
	cvReleaseMemStorage(&mem);
}

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

/* This finds the mean slope for the given sequence of lines.
Actually it returns the average slope for the two extreme slopes.*/
void meanLine(CvSeq *lines, double *dst_m) {
	int max_i, min_i;
	double max_m = 0, min_m = 0;
	for(int i = 0; i < lines->total; i++) {
		CvPoint *line = (CvPoint*)cvGetSeqElem(lines, i);
		
		double m = ((double)(line[1].y - line[0].y + 0.))/(line[1].x - line[0].x);
		if(i == 0) {
			max_m = min_m = m;
			max_i = min_i = 0;
		} else {
			if(m > max_m) {
				max_m = m;
				max_i = i;
			} else if(m < min_m) {
				min_m = m;
				min_i = i;
			}
		}
	}

	if(!(min_m >= DBL_MIN && min_m <= DBL_MAX)) {
		*dst_m = max_m;
	} else if(!(max_m >= DBL_MIN && max_m <= DBL_MAX)) {
		*dst_m = min_m;
	} else {
		*dst_m = (min_m + max_m)/2;
	}
}

/* This finds the mean center-of-mass for the given sequence of lines */
void meanCM(CvSeq *lines, CvPoint *dst_cm) {
	double cm_x = 0, cm_y = 0;

	if(lines->total == 0) {
		dst_cm->x = 0;
		dst_cm->y = 0;
		return;
	}

	for(int i = 0; i < lines->total; i++) {
		CvPoint *line = (CvPoint*)cvGetSeqElem(lines, i);
		
		cm_x += line[0].x + line[1].x;
		cm_y += line[0].y + line[1].y;
	}

	dst_cm->x = (int)(cm_x/(2*lines->total));
	dst_cm->y = (int)(cm_y/(2*lines->total));
}

/* This uses findCue() and marks the results on the image (around the white
ball) */
void markCue(IplImage *src, CvPoint2D32f white_center, float white_radius) {
	double cue_m;
	CvPoint cue_cm;
	findCue(src, &cue_m, &cue_cm);

	if(cue_cm.x == 0 && cue_cm.y == 0)
		return;
	
	int cue_sign = (int)(fabs(cue_m)/cue_m);
	cue_m = fabs(cue_m);

	double length = 100;
	CvPoint p1, p2;

	int x_sign = 1;
	if(cue_cm.x < white_center.x) {
		// if we're pointing from the left of the ball
		x_sign = -1;
	}
	int y_sign = cue_sign;

	white_radius = white_radius + 5*downscale_factor;
	p1.x = cvRound(white_center.x + x_sign*(white_radius/sqrt(cue_m+1)));
	p1.y = cvRound(white_center.y + y_sign*(p1.x - white_center.x)*cue_m);

	p2.x = p1.x + x_sign*cvRound(length/sqrt(cue_m+1));
	p2.y = p1.y + y_sign*cvRound((p2.x-p1.x)*cue_m);

	cvLine(src, p1, p2, cvScalar(255, 0, 0), 3);
}

/* This filters lines with not-near-enough-to-template histogram around them */
void filterLinesByHistogram(IplImage *src, CvSeq *lines, int nbins, int width,
							 double corr_thresh) {
	bool debug = false;

	CvHistogram *cueHist = cueHistogram(width, nbins);

	if(debug) {
		cvNamedWindow("Histogram", CV_WINDOW_AUTOSIZE);
		cvNamedWindow("Stick", CV_WINDOW_AUTOSIZE);
		
		cvNamedWindow("Template Histogram", CV_WINDOW_AUTOSIZE);
		drawHistogram("Template Histogram", cueHist, nbins);
	}

	for(int i = 0; i < lines->total; i++) {
		CvPoint *line = (CvPoint*)cvGetSeqElem(lines, i);

		CvHistogram *hist = hueHistFromLine(src, line[0], line[1], width,
			nbins);

		double corr = cvCompareHist(hist, cueHist, CV_COMP_CORREL);
		if(debug) {
			cout<<corr<<endl;
		}

		if(corr > corr_thresh) {
			if(debug) {
				cvLine( src, line[0], line[1], cvScalar(255), 2);
			}
		} else {
			cvSeqRemove(lines, i);
			i--;
			if(debug) {
				cvLine( src, line[0], line[1], cvScalar(0, 0, 255), 2);
			}
		}

		if(debug) {
			drawHistogram("Histogram", hist, nbins);
			cvShowImageWrapper("Stick", src);
			cvWaitKey(0);
		}

		cvReleaseHist(&hist);
	}
	cvReleaseHist(&cueHist);

	if(debug) {
		cvDestroyWindow("Histogram");
		cvDestroyWindow("Stick");
		cvDestroyWindow("Template Histogram");
	}
}

/* This finds the center-of-mass and slope of the cue in the given image */
void findCue(IplImage *src, double *cue_m, CvPoint *cue_cm) {
	IplImage *gray = createBlankCopy(src, 1);
	cvCvtColor(src, gray, CV_BGR2GRAY);

	IplImage *edge = createBlankCopy(gray);
	cvCanny(gray, edge, 150, 50);

	CvMemStorage *storage = cvCreateMemStorage(0);
	CvSeq *lines = cvHoughLines2(edge, storage, CV_HOUGH_PROBABILISTIC, 0.5, 0.5*CV_PI/180, 80, 50, 50);

	// filter lines not on the board
	drawBorders(src, 1);
	filterLinesOnTable(lines);

	// filter very similar lines
	double m_thresh = 0.05;
	double n_thresh = 10;
	filterSimiliarLines(lines, m_thresh, n_thresh);

	// filter the cue using histogram
	int nbins = 30;
	int width = 5;
	double corr_thresh = .55;
	filterLinesByHistogram(src, lines, nbins, width, corr_thresh);

	// compute the mean line
	meanLine(lines, cue_m);
	// compute the center of mass
	meanCM(lines, cue_cm);

	cvReleaseImage(&gray);
	cvReleaseImage(&edge);
	cvReleaseMemStorage(&storage);
}

/* Mouse callback wrapper that prints the color of the clicked point. param
is the img to work on */
void findColorAroundMouse(int event, int x, int y, int flags, void *param) {
	if(event != CV_EVENT_LBUTTONUP)
		return;

	IplImage *img = (IplImage *)param;

	CvScalar color = cvGet2D(img, y, x);
	for(int i=0; i<img->nChannels; i++) {
		cout<<color.val[i]<<" ";
	}
	cout<<endl;

	IplImage *temp = cvCloneImage(img);
	cvCircle(temp, cvPoint(x, y), 1, cvScalar(0xff,0,0), 2);
	cvShowImageWrapper("MouseWrapper", temp);
	cvReleaseImage(&temp);
}

/* Mouse callback wrapper that prints the position of the clicked point.
param is the img to work on */
void findPosAroundMouse(int event, int x, int y, int flags, void *param) {
	if(event != CV_EVENT_LBUTTONUP)
		return;

	IplImage *img = (IplImage *)param;

	cout<<"(x,y) = ("<<x<<", "<<y<<")\n";

	IplImage *temp = cvCloneImage(img);
	cvCircle(temp, cvPoint(x, y), 1, cvScalar(0xff,0,0), 2);
	cvShowImageWrapper("MouseWrapper", temp);
	cvReleaseImage(&temp);
}

/* Mouse callback wrapper for findBallAround. param is the img to work on */
void findBallAroundMouse(int event, int x, int y, int flags, void *param) {
	if(event != CV_EVENT_LBUTTONUP)
		return;

	IplImage *img = (IplImage *)param;

	x = (int)(((double)x)/1.5625);
	y = (int)(((double)y)/1.5625);

	// find the ball parameters
	CvPoint2D32f center;
	float radius;
	findBallAround(img, cvPoint(x,y), &center, &radius);

	// print an overlay image of the found circle
	IplImage *overlay = createBlankCopy(img);
	IplImage *overlay_drawing = createBlankCopy(overlay, 1);
	IplImage *overlay_blank = createBlankCopy(overlay, 1);

	cvSet(overlay_blank, cvScalar(0));

	cvCircle(overlay_drawing, cvPoint(cvRound(center.x), cvRound(center.y)),
		cvRound(radius), cvScalar(0xff), 2);
	int line_len = 5;
	cvLine(overlay_drawing, cvPoint(cvRound(center.x)-line_len,
		cvRound(center.y)), cvPoint(cvRound(center.x)+line_len,
		cvRound(center.y)), cvScalar(0xff), 2);
	cvLine(overlay_drawing, cvPoint(cvRound(center.x),
		cvRound(center.y)-line_len), cvPoint(cvRound(center.x),
		cvRound(center.y)+line_len), cvScalar(0xff), 2);

	cvMerge(overlay_blank, overlay_blank, overlay_drawing, 0, overlay);

	IplImage *temp = cvCloneImage(img);
	cvAddWeighted(img, 0.5, overlay, 1, 0, temp);

	cvReleaseImage(&overlay);
	cvReleaseImage(&overlay_drawing);
	cvReleaseImage(&overlay_blank);

	// DEBUG -- print a cross where the mouse is
	/*IplImage *temp = cvCloneImage(img);

	cvLine(temp, cvPoint(x-5, y),
		cvPoint(x+5, y), cvScalar(0xff), 2);
	cvLine(temp, cvPoint(x, y-5),
		cvPoint(x, y+5), cvScalar(0xff), 2);*/

	cvShowImageWrapper("MouseWrapper", temp);

	cvReleaseImage(&temp);
}

/* Get the background from multiple background files and avergage them */
IplImage* getBackground() {
	IplImage *src1 = cvLoadImage("C:\\Projecton\\Test\\Testing\\bg1.jpg");
	IplImage *src2 = cvLoadImage("C:\\Projecton\\Test\\Testing\\bg1.jpg");
	IplImage *src3 = cvLoadImage("C:\\Projecton\\Test\\Testing\\bg1.jpg");

	IplImage *bg = cvCreateImage(cvGetSize(src1), src1->depth,
		src1->nChannels);

	cvAddWeighted(src1, 1./2., src2, 1./2., 0., bg);
	cvAddWeighted(bg, 2./3., src3, 1./3., 0., bg);

	cvReleaseImage(&src1);
	cvReleaseImage(&src2);
	cvReleaseImage(&src3);

	return bg;
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

/* Normalize the image for debugging, so it could fit on the screen. */
void normalize(IplImage* &img, CvRect crop, CvSize size) {
	cvSetImageROI(img, crop);
	IplImage *img_new = cvCreateImage(size, img->depth, img->nChannels);
	cvPyrDown(img, img_new);
	cvReleaseImage(&img);
	img = img_new;
}

/* Set the channels a,b,c of src to:
a/(a+b+c), b/(a+b+c), c/(a+b+c).
This essentially normalizes the brightness and contrast, against global
lighting changes.*/
void equalize(IplImage *src, IplImage *dst) {
	IplImage *a = createBlankCopy(src, 1, IPL_DEPTH_8U);
	IplImage *b = createBlankCopy(src, 1, IPL_DEPTH_8U);
	IplImage *c = createBlankCopy(src, 1, IPL_DEPTH_8U);

	cvSplit(src, a, b, c, NULL);

	IplImage *tmp = createBlankCopy(src, 1, IPL_DEPTH_16U);
	IplImage *sum = createBlankCopy(src, 1, IPL_DEPTH_16U);

	cvConvertScale(a, sum);
	cvConvertScale(b, tmp);
	cvAddWeighted(sum, 1./3., tmp, 1./3., 0., sum);
	cvConvertScale(c, tmp);
	cvAddWeighted(sum, 1, tmp, 1./3., 0., sum);

	cvDiv(tmp, sum, tmp, 255);
	cvConvertScale(tmp, c);

	cvConvertScale(b, tmp);
	cvDiv(tmp, sum, tmp, 255);
	cvConvertScale(tmp, b);

	cvConvertScale(a, tmp);
	cvDiv(tmp, sum, tmp, 255);
	cvConvertScale(tmp, a);

	cvReleaseImage(&a);
	cvReleaseImage(&b);
	cvReleaseImage(&c);
	cvReleaseImage(&tmp);
	cvReleaseImage(&sum);

	cvMerge(a, b, c, NULL, dst);
}

/* Find circles using Hough Transform, and draw them on the returned image,
overlayed on the original image. */
IplImage *findCircles(IplImage *src, int canny, int thresh, int min_dist,
					  int min_radius, int max_radius) {
	CvMemStorage* storage = cvCreateMemStorage(0);

    // Copy src
	IplImage *img = createBlankCopy(src);
	cvCopyImage(src,img);
	IplImage *dst = createBlankCopy(src, 3);
	cvMerge(src, src, src, NULL, dst);

    cvSmooth(img, img, CV_GAUSSIAN, 3, 3); // LPF
    // find circles
    CvSeq* results = cvHoughCircles(img, storage, CV_HOUGH_GRADIENT, 1,
		min_dist, canny, thresh);

    // draw circles
    for(int i = 0; i < results->total; i++ ) {
            float* p = (float*)cvGetSeqElem(results, i);
            CvPoint pt = cvPoint(cvRound(p[0]), cvRound(p[1]));
            
            // skip according to radius
            if(cvRound(p[2]) < min_radius || cvRound(p[2]) > max_radius)
                    continue;

            // circle
            cvCircle(dst, pt, cvRound(p[2]), CV_RGB(0xff,0,0), 2);

            // cross
            int line_len = 5;
            cvLine(dst, cvPoint(cvRound(p[0])-line_len, cvRound(p[1])),
                    cvPoint(cvRound(p[0])+line_len, cvRound(p[1])),
					CV_RGB(0xff,0,0), 2);
            cvLine(dst, cvPoint(cvRound(p[0]), cvRound(p[1])-line_len),
                    cvPoint(cvRound(p[0]), cvRound(p[1])+line_len),
					CV_RGB(0xff,0,0), 2);
    }

    cvReleaseMemStorage(&storage);
	cvReleaseImage(&img);

	return dst;
}

/* Compute a mask according to a threshold on the difference from a certain
color (thresh may be different for each channel) */
void colorThresh(IplImage *src_image, IplImage *dst_mask, CvScalar color, 
                                 CvScalar thresh) {
        int i;
        int n_channels = src_image->nChannels;

        // compute difference
        IplImage *tmp = cvCreateImage(cvGetSize(src_image), src_image->depth, 
                src_image->nChannels);
        cvAbsDiffS(src_image, tmp, color);
        
        // split to channels
        IplImage *channels[4] = {NULL, NULL, NULL, NULL};
        for(i=0; i<n_channels; i++) {
                channels[i] = cvCreateImage(cvGetSize(tmp), tmp->depth, 1);
        }
        cvSplit(tmp, channels[0], channels[1], channels[2], channels[3]);
        cvReleaseImage(&tmp);

        // zero the mask
        cvSet(dst_mask, cvScalar(1));

        // threshold every channel and merge the thresholds
        for(i=0; i<n_channels; i++) {
                cvThreshold(channels[i], channels[i], thresh.val[i], 1,
					CV_THRESH_BINARY);
                cvMul(channels[i], dst_mask, dst_mask);
                cvReleaseImage(&channels[i]);
        }
}

/* Finds the center and radius of the maximal circle containing the point
given, and not containing any out-of-thresh point in the given image.

Works by starting with a little circle around the point given, and tries to
enlarge it. If it meets any out-of-thresh pixel, it tries to move: left, right,
up, down, or diagonally and continue growing. When we can't move without
meeting an out-of-thresh pixel, we're done.*/
void findMaxContainedCircle(IplImage *bw, CvPoint inside, 
							CvPoint2D32f *f_center, float *f_radius,
							int thresh) {
	CvPoint2D32f center = cvPoint2D32f(inside.x, inside.y);
	float radius = 1;

	int height = cvGetSize(bw).height;
	int width = cvGetSize(bw).width;

	int maxIter = 2500;
	float radial_step_size = 0.25;
	float spatial_steps[] = {1, 0.5, 0.25};
	int spatial_steps_len = 3;
	double angles[] = {0*PI/8, 1*PI/8, 2*PI/8, 3*PI/8, 4*PI/8, 5*PI/8, 6*PI/8, 7*PI/8,
		8*PI/8, 9*PI/8, 10*PI/8, 11*PI/8, 12*PI/8, 13*PI/8, 14*PI/8, 15*PI/8};
	int angles_len = 16;

	//cvNamedWindow("Wnd-0", CV_WINDOW_AUTOSIZE);
	
	int start_x, start_y, end_x, end_y;
	int x,y, i;
	bool found;
	float dist2;
	float radius2 = 1;
	int movement = 0;
	CvPoint2D32f movement_center;
	for(i=0; i < maxIter; i++) {
		found = false; // have we found a non-zero yet?
		// run on the square containing the circle 
		start_x = MAX(cvRound(center.x - radius), 0);
		start_y = MAX(cvRound(center.y - radius), 0);
		end_x = MIN(cvRound(center.x + radius), width-1);
		end_y = MIN(cvRound(center.y + radius), height-1);
		for(x=start_x; x<=end_x && !found; x++) {
			for(y=start_y; y<=end_y && !found; y++) {
				if(cvGet2D(bw, y, x).val[0] >= thresh) {
					// if there's a non-zero in it, it may be inside the circle!
					dist2 = (x-center.x)*(x-center.x) +
						(y-center.y)*(y-center.y);
					if(dist2 <= radius2) {
						// inside the circle
						found = true;
					}
				}
			}
		}

		if(found) {	// we found a non-zero
			if(movement == 0)
				movement_center = center;

			movement += 1; // try the next direction

			if(movement >= angles_len*spatial_steps_len) {	// nothing worked
				// set center to the last working center
				center = movement_center;
				// return
				break;
			}

			center.x = movement_center.x +
				spatial_steps[movement/angles_len]*
				(float)cos(angles[movement%angles_len]);
			center.y = movement_center.y +
				spatial_steps[movement/angles_len]*
				(float)sin(angles[movement%angles_len]);
		} else { // we've not found a non-zero
			// enlarge the radius
			radius += radial_step_size;
			radius2 = radius*radius;
			movement = 0; // new try
		}

		// DEBUG
		/*IplImage *temp = cvCloneImage(bw);
		cvRectangle(temp, cvPoint(start_x, start_y), cvPoint(end_x, end_y),
			cvScalar(0xaa));
		cvCircle(temp, cvPoint(cvRound(center.x), cvRound(center.y)),
			cvRound(radius), cvScalar(0xaa));
		cvShowImageWrapper("Wnd-0", temp);
		cout<<movement<<": "<<center.x<<" "<<center.y<<" -- "<<radius<<endl;
		cvWaitKey(0);*/
		// END DEBUG
	}

	//cvDestroyWindow("Wnd-0");

	// return the found parameters
	*f_radius = radius;
	f_center->x = center.x;
	f_center->y = center.y;
}

/* This tries to find the center and radius of the ball which contains
the given point, in the image. */
void findBallAround(IplImage *src, CvPoint inside, CvPoint2D32f *center,
					float *radius, bool debug) {

	// crop around the ball
	int max_radius = 27;
	int min_radius = 23;

	IplImage *cropped = cvCreateImage(
		cvSize(4*max_radius, 4*max_radius), src->depth, src->nChannels);

	int src_width = cvGetSize(src).width;
	int src_height = cvGetSize(src).height;
	CvPoint crop_corner = cvPoint(MAX(inside.x-2*max_radius,0),
		MAX(inside.y-2*max_radius,0));
	cvSetImageROI( src, 
		cvRect(crop_corner.x, crop_corner.y, 
		MIN(4*max_radius,src_width), MIN(4*max_radius,src_height)));
	cvCopy(src, cropped);
	cvResetImageROI(src);
	CvPoint crop_inside = cvPoint(inside.x - crop_corner.x,
		inside.y - crop_corner.y);

	// compute a laplacian of the channels and merge it
		// split to channels
	IplImage *cropped_a = createBlankCopy(cropped, 1, IPL_DEPTH_8U);
	IplImage *cropped_b = createBlankCopy(cropped, 1, IPL_DEPTH_8U);
	IplImage *cropped_c = createBlankCopy(cropped, 1, IPL_DEPTH_8U);
	cvSplit(cropped, cropped_a, cropped_b, cropped_c, NULL);

		// create output channels (16bit signed)
	IplImage *grad_a16 = createBlankCopy(cropped, 1, IPL_DEPTH_16S);
	IplImage *grad_b16 = createBlankCopy(cropped, 1, IPL_DEPTH_16S);
	IplImage *grad_c16 = createBlankCopy(cropped, 1, IPL_DEPTH_16S);

		// compute
	cvLaplace(cropped_a, grad_a16, 5);
	cvLaplace(cropped_b, grad_b16, 5);
	cvLaplace(cropped_c, grad_c16, 5);

		// scale back to 8bit unsigned
	IplImage *grad_a8 = createBlankCopy(cropped, 1, IPL_DEPTH_8U);
	IplImage *grad_b8 = createBlankCopy(cropped, 1, IPL_DEPTH_8U);
	IplImage *grad_c8 = createBlankCopy(cropped, 1, IPL_DEPTH_8U);

	double scale,shift, minVal = .0, maxVal = .0, min = 0, max = 255;

	cvMinMaxLoc( grad_a16, &minVal, &maxVal, NULL, NULL, 0);
	scale = (max - min)/(maxVal-minVal);
	shift = -minVal * scale + min;
	cvConvertScale( grad_a16, grad_a8, scale, shift );

	cvMinMaxLoc( grad_b16, &minVal, &maxVal, NULL, NULL, 0);
	scale = (max - min)/(maxVal-minVal);
	shift = -minVal * scale + min;
	cvConvertScale( grad_b16, grad_b8, scale, shift );

	cvMinMaxLoc( grad_c16, &minVal, &maxVal, NULL, NULL, 0);
	scale = (max - min)/(maxVal-minVal);
	shift = -minVal * scale + min;
	cvConvertScale( grad_c16, grad_c8, scale, shift );

		// release temporaries
	cvReleaseImage(&cropped_a);
	cvReleaseImage(&cropped_b);
	cvReleaseImage(&cropped_c);
	cvReleaseImage(&grad_a16);
	cvReleaseImage(&grad_b16);
	cvReleaseImage(&grad_c16);

		// merge the results (into a "gray" laplacian)
	IplImage *grad = createBlankCopy(grad_a8);
	cvAddWeighted(grad_a8, 0.299, grad_b8, 0.587, 0, grad);
	cvAddWeighted(grad, 0.886, grad_c8, 0.114, 0, grad);

		// change to BW
	cvSmooth(grad, grad, CV_MEDIAN, 3);
	cvThreshold(grad, grad, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);
	if(cvGet2D(grad, crop_inside.x, crop_inside.y).val[0] == 255) {
		cvAbsDiffS(grad, grad, cvScalar(255));
	}

		// find the circle containing the given point in the BW image
	findMaxContainedCircle(grad, crop_inside, center,
		radius, 1);

	if(debug) {
		// an overlay of the BW over the crop
		IplImage *thresh_on_cropped = cvCloneImage(cropped);
		IplImage *overlay = createBlankCopy(cropped);
		IplImage *blank = createBlankCopy(grad);
		cvSet(blank, cvScalar(0));
		cvMerge(blank, blank, grad, 0, overlay);
		cvAddWeighted(cropped, 0.5, overlay, 0.5, 0, thresh_on_cropped);
		cvCircle(thresh_on_cropped, crop_inside, 1, cvScalar(0xff,0,0));
		cvReleaseImage(&overlay);
		cvReleaseImage(&blank);

		// overlay the found circle over the crop
		IplImage *found_circ = cvCloneImage(cropped);
		cvCircle(found_circ, cvPoint(cvRound(center->x), cvRound(center->y)),
			cvRound(*radius), cvScalar(0xff), 1);
		int line_len = 5;
		cvLine(found_circ, cvPoint(cvRound(center->x)-line_len, cvRound(center->y)),
			cvPoint(cvRound(center->x)+line_len, cvRound(center->y)), cvScalar(0xaa), 1);
		cvLine(found_circ, cvPoint(cvRound(center->x), cvRound(center->y)-line_len),
			cvPoint(cvRound(center->x), cvRound(center->y)+line_len), cvScalar(0xaa), 1);

		// display the channels and the overlayed crop
		cvNamedWindow("Laplacian Threshold", CV_WINDOW_AUTOSIZE);
		cvShowImageWrapper("Laplacian Threshold", grad);
		cvNamedWindow("Laplacian Threshold Overlay", CV_WINDOW_AUTOSIZE);
		cvShowImageWrapper("Laplacian Threshold Overlay", thresh_on_cropped);
		cvReleaseImage(&thresh_on_cropped);
		cvNamedWindow("Found Circle", CV_WINDOW_AUTOSIZE);
		cvShowImageWrapper("Found Circle", found_circ);
		cvReleaseImage(&found_circ);


		cvWaitKey(0);

		cvDestroyWindow("Laplacian Threshold");
		cvDestroyWindow("Laplacian Threshold Overlay");
		cvDestroyWindow("Found Circle");
	}

	// normalize the results to before the crop
	center->x += crop_corner.x;
	center->y += crop_corner.y;

	cvReleaseImage(&grad_a8);
	cvReleaseImage(&grad_b8);
	cvReleaseImage(&grad_c8);
	cvReleaseImage(&grad);
	cvReleaseImage(&cropped);
}
