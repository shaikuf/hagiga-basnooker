#include <highgui.h>
#include <cv.h>
#include <math.h>
#include <iostream>
#include "camera_interface.h"

using namespace std;

IplImage* getBackground() {
	IplImage *src1 = cvLoadImage("C:\\Projecton\\Test\\Testing\\bg1.jpg");
	IplImage *src2 = cvLoadImage("C:\\Projecton\\Test\\Testing\\bg1.jpg");
	IplImage *src3 = cvLoadImage("C:\\Projecton\\Test\\Testing\\bg1.jpg");

	IplImage *bg = cvCreateImage(cvGetSize(src1), src1->depth, src1->nChannels);

	cvAddWeighted(src1, 1./2., src2, 1./2., 0., bg);
	cvAddWeighted(bg, 2./3., src3, 1./3., 0., bg);

	cvReleaseImage(&src1);
	cvReleaseImage(&src2);
	cvReleaseImage(&src3);

	return bg;
}

IplImage *createBlankCopy(IplImage *src, int channels = -1, int depth = -1) {
	if(depth == -1)
		depth = src->depth;
	if(channels == -1)
		channels = src->nChannels;

	return cvCreateImage(cvGetSize(src), depth, channels);
}

void normalize(IplImage* &img, CvRect crop, CvSize size) {
	cvSetImageROI(img, crop);
	IplImage *img_new = cvCreateImage(size, img->depth, img->nChannels);
	cvPyrDown(img, img_new);
	cvReleaseImage(&img);
	img = img_new;
}

void Equalize(IplImage *src, IplImage *dst) {
	IplImage *a = createBlankCopy(src, 1, IPL_DEPTH_8U);
	IplImage *b = createBlankCopy(src, 1, IPL_DEPTH_8U);
	IplImage *c = createBlankCopy(src, 1, IPL_DEPTH_8U);

	cvSplit(src, a, b, c, NULL);

	IplImage *tmp = createBlankCopy(src, 1, IPL_DEPTH_16U);
	IplImage *sum = createBlankCopy(src, 1, IPL_DEPTH_16U);

	cvConvertScale(a, sum);
	cvConvertScale(b, tmp);
	cvAddWeighted(sum, 1./2., tmp, 1./2., 0., sum);
	cvConvertScale(c, tmp);
	cvAddWeighted(sum, 2./3., tmp, 1./3., 0., sum);

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

IplImage *findCircles(IplImage *src, int canny, int thresh, int min_dist, int min_radius, int max_radius) {
	CvMemStorage* storage = cvCreateMemStorage(0);

    // Copy src
	IplImage *img = createBlankCopy(src);
	cvCopyImage(src,img);
	IplImage *dst = createBlankCopy(src, 3);
	cvMerge(src, src, src, NULL, dst);

    cvSmooth(img, img, CV_GAUSSIAN, 3, 3); // LPF
    // find circles
    CvSeq* results = cvHoughCircles(img, storage, CV_HOUGH_GRADIENT, 1, min_dist, canny, thresh);

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
                    cvPoint(cvRound(p[0])+line_len, cvRound(p[1])), CV_RGB(0xff,0,0), 2);
            cvLine(dst, cvPoint(cvRound(p[0]), cvRound(p[1])-line_len),
                    cvPoint(cvRound(p[0]), cvRound(p[1])+line_len), CV_RGB(0xff,0,0), 2);
    }

    cvReleaseMemStorage(&storage);
	cvReleaseImage(&img);

	return dst;
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

int main(int argc, char* argv[])
{
	CvRect crop = cvRect(0, 0, 1600, 850);
	CvSize size = cvSize(800, 425);

	IplImage *bg;

    bg = getBackground();
	normalize(bg, crop, size);
	//Equalize(bg, bg);

	IplImage *white_ball = cvLoadImage("C:\\Projecton\\Test\\Testing\\Picture 16.jpg"); 
	normalize(white_ball, crop, size);
	//Equalize(white_ball, white_ball);

	IplImage *diff = createBlankCopy(white_ball);
	cvAbsDiff(white_ball, bg, diff);

	// Split to channels
	cvCvtColor(diff, diff, CV_BGR2YCrCb);
	IplImage *a = createBlankCopy(diff, 1);
	IplImage *b = createBlankCopy(diff, 1);
	IplImage *c = createBlankCopy(diff, 1);
	cvSplit(diff, a, b, c, NULL);

    // Display channels
	cvNamedWindow("Wnd-1", CV_WINDOW_AUTOSIZE);
    cvShowImage("Wnd-1", diff);
    cvNamedWindow("Wnd0", CV_WINDOW_AUTOSIZE);
    cvShowImage("Wnd0", a);
    cvNamedWindow("Wnd1", CV_WINDOW_AUTOSIZE);
    cvShowImage("Wnd1", b);
    cvNamedWindow("Wnd2", CV_WINDOW_AUTOSIZE);
    cvShowImage("Wnd2", c);

    cvWaitKey(0);

	// Free everything
	cvDestroyWindow("Wnd-1");
    cvDestroyWindow("Wnd0");
    cvDestroyWindow("Wnd1");
    cvDestroyWindow("Wnd2");


	cvReleaseImage(&bg);
	cvReleaseImage(&white_ball);
	cvReleaseImage(&diff);

	return 0;
}

