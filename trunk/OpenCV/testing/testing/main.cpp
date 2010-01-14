#include <iostream>
#include <highgui.h>
#include <cv.h>
#include <time.h>
#include <limits.h>
#include "main.h"
#include "misc.h"
#include "cue.h"
#include "balls.h"

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


