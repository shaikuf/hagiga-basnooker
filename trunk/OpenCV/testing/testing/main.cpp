#include <highgui.h>
#include <cv.h>
#include <math.h>
#include <iostream>
#include "camera_interface.h"

using namespace std;

void ColorThresh(IplImage *src_image, IplImage *dst_mask, CvScalar color, 
				 CvScalar thresh);
void AvgSdvRect(IplImage *src, CvScalar *mean, CvScalar *std_dev, CvRect rect);

int main(int argc, char* argv[])
{
	IplImage *src = cvLoadImage("C:\\Projecton\\Test\\table1.jpg", 1);
	CvRect crop = cvRect(0, 230, 1500, 800);

	cvSetImageROI(src, crop);

	// Change colorspace
	cvCvtColor(src, src, CV_BGR2HSV);

	// Split to channels
	IplImage *a = cvCreateImage(cvGetSize(src), src->depth, 1);
	IplImage *b = cvCreateImage(cvGetSize(src), src->depth, 1);
	IplImage *c = cvCreateImage(cvGetSize(src), src->depth, 1);
	cvSplit(src, a, b, c, NULL);

	//cvSmooth(src, src, CV_GAUSSIAN, 3, 3); // Gaussian Average 3x3

	//cvCanny(src, src, 100, 50); // Canny Edge Detection

	// Find the BG color
	/*CvScalar mean, std_dev;
	AvgSdvRect(c, &mean, &std_dev, cvRect(25, 0, 25, 25));
	cout<<"mean: "<<mean.val[0]endl;
	cout<<"std-dev: "<<std_dev.val[0]<<endl;*/

	//ColorThresh(c, c, mean, cvScalar(15)); // Threshold on dist. from BG
	//cvConvertScale(c, c, 255, 0); // Make the 0/1 mask 0/255
	
	// Zero outside the mask
	/*cvNot(c, c);
	cvSet(src, cvScalar(0), c);
	cvCvtColor(src, src, CV_HSV2BGR);*/

	// Display channels
	cvNamedWindow("Wnd0", CV_WINDOW_AUTOSIZE);
	cvShowImage("Wnd0", a);
	cvNamedWindow("Wnd1", CV_WINDOW_AUTOSIZE);
	cvShowImage("Wnd1", b);
	cvNamedWindow("Wnd2", CV_WINDOW_AUTOSIZE);
	cvShowImage("Wnd2", c);

	cvWaitKey(0);

	cvDestroyWindow("Wnd0");
	cvDestroyWindow("Wnd1");
	cvDestroyWindow("Wnd2");
	cvReleaseImage(&a);
	cvReleaseImage(&b);
	cvReleaseImage(&c);
	cvReleaseImage(&src);

	// Find circles
	/*CvMemStorage* storage = cvCreateMemStorage(0);

	// Copy src
	IplImage *img = cvCreateImage(cvGetSize(c), c->depth, 1);
	cvCopy(c, img);

	cvSmooth(img, img, CV_GAUSSIAN, 3, 3); // LPF
	int x = 50; // canny parameter
	cvCanny(img, img, x, x/2); // edge detection
	// find circles
	CvSeq* results = cvHoughCircles(img, storage, CV_HOUGH_GRADIENT, 1, 20, x, 20);

	// draw circles
	for(int i = 0; i < results->total; i++ ) {
		float* p = (float*)cvGetSeqElem(results, i);
		CvPoint pt = cvPoint(cvRound(p[0]), cvRound(p[1]));
		
			// skip according to radius
		if(cvRound(p[2]) < 18 || cvRound(p[2]) > 26) {
			continue;
		} else {
			cout<<cvRound(p[2])<<endl;
		}

		// circle
		cvCircle(src, pt, cvRound(p[2]), CV_RGB(0xff,0,0), 2);

		// cross
		int line_len = 5;
		cvLine(src, cvPoint(cvRound(p[0])-line_len, cvRound(p[1])),
			cvPoint(cvRound(p[0])+line_len, cvRound(p[1])), CV_RGB(0xff,0,0), 2);
		cvLine(src, cvPoint(cvRound(p[0]), cvRound(p[1])-line_len),
			cvPoint(cvRound(p[0]), cvRound(p[1])+line_len), CV_RGB(0xff,0,0), 2);
	}

	cvReleaseMemStorage(&storage);

	cvNamedWindow("Wnd3", CV_WINDOW_AUTOSIZE);
	cvShowImage("Wnd3", img);
	cvNamedWindow("Wnd4", CV_WINDOW_AUTOSIZE);
	cvShowImage("Wnd4", src);

	cvWaitKey(0);

	cvDestroyWindow("Wnd3");
	cvDestroyWindow("Wnd4");


	cvSaveImage("C:\\Projecton\\Test\\1-canny.jpg", img);
	cvSaveImage("C:\\Projecton\\Test\\1-circles.jpg", src);

	cvReleaseImage(&img);
	cvReleaseImage(&src);*/

	return 0;
}

// compute avg. and std-dev on a rectangular part of the image
void AvgSdvRect(IplImage *src, CvScalar *mean, CvScalar *std_dev, CvRect rect) {
	// create mask
	IplImage *mask = cvCreateImage(cvGetSize(src), IPL_DEPTH_8U, 1);
	cvSet(mask, cvScalar(0));

	cvRectangle(mask, cvPoint(rect.x,rect.y),cvPoint(rect.x+rect.width,
		rect.y+rect.height), cvScalar(1), CV_FILLED);

	// compute
	cvAvgSdv(src, mean, std_dev, mask);

	cvReleaseImage(&mask);
}

// compute a mask according to a threshold on the difference from a certain
// color (thresh may be different for each channel)
void ColorThresh(IplImage *src_image, IplImage *dst_mask, CvScalar color, 
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
		cvThreshold(channels[i], channels[i], thresh.val[i], 1, CV_THRESH_BINARY);
		cvMul(channels[i], dst_mask, dst_mask);
		cvReleaseImage(&channels[i]);
	}
}