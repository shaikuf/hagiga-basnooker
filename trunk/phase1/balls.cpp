#include <cv.h>
#include <highgui.h>
#include "balls.h"
#include "misc.h"

/* This finds the ball matching the given template on the image */
void findBall(IplImage *img, IplImage *templ, CvPoint2D32f *center,
			  float *radius, bool debug) {
	// find the template
	CvPoint p;
	findTemplate(img, templ, &p, debug);

	center->x = p.x;
	center->y = p.y;
	*radius = templ->width/2;
	return;

	//white = {0.299, 0.587, 0.114}

	// find the ball parameters
	findBallAround(img, p, templ->width/2, cvGet2D(templ, templ->height/2,
		templ->width/2).val, center, radius, debug);
}

/* This uses findBall() and marks the results on the image */
void markBall(IplImage *img, CvPoint2D32f center, float radius,
			  CvScalar color, bool circle) {

	CvPoint center_i = cvPointFrom32f(center);

	// print an overlay image of the found circle
	IplImage *overlay_drawing = createBlankCopy(img, 1);
	cvSet(overlay_drawing, cvScalar(0));

	if(circle) {
		cvCircle(overlay_drawing, cvPoint(cvRound(center_i.x),
				cvRound(center_i.y)),	cvRound(radius), cvScalar(0xff), 1);
	}

    int line_len = 5;
	cvLine(overlay_drawing, cvPoint(cvRound(center_i.x)-line_len,
		cvRound(center_i.y)), cvPoint(cvRound(center_i.x)+line_len,
		cvRound(center_i.y)), cvScalar(0xff), 1);
	cvLine(overlay_drawing, cvPoint(cvRound(center_i.x),
		cvRound(center_i.y)-line_len), cvPoint(cvRound(center_i.x),
		cvRound(center_i.y)+line_len), cvScalar(0xff), 1);

	cvSet(img, color, overlay_drawing);

	cvReleaseImage(&overlay_drawing);
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
void findBallAround(IplImage *src, CvPoint inside, int template_radius,
					double color[], CvPoint2D32f *center, float *radius,
					bool debug) {

	// crop around the ball

	float mul = 3;

	IplImage *cropped = cvCreateImage(
		cvSize(mul*template_radius, mul*template_radius), src->depth, src->nChannels);

	int src_width = cvGetSize(src).width;
	int src_height = cvGetSize(src).height;

	CvPoint crop_corner = cvPoint(MAX(inside.x-(mul/2)*template_radius,0),
		MAX(inside.y-(mul/2)*template_radius,0));
	cvSetImageROI( src, 
		cvRect(crop_corner.x, crop_corner.y, 
		MIN(mul*template_radius,src_width), MIN(mul*template_radius,src_height)));
	cvCopy(src, cropped);
	cvResetImageROI(src);
	CvPoint crop_inside = cvPoint(inside.x - crop_corner.x,
		inside.y - crop_corner.y);

	// preprocessing

	IplImage *temp = createBlankCopy(cropped);
	cvSmooth(cropped, cropped, CV_GAUSSIAN, 5);
	cvSmooth(cropped, temp, CV_BILATERAL, 0, 0, 23, 7);
	cvCopy(temp, cropped);
	cvReleaseImage(&temp);

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
	cvAddWeighted(grad_a8, color[0]/(color[0]+color[1]), grad_b8,
		color[1]/(color[0]+color[1]), 0, grad);
	cvAddWeighted(grad, (color[0]+color[1])/(color[0]+color[1]+color[2]),
		grad_c8, color[2]/(color[0]+color[1]+color[2]), 0, grad);

	cvNamedWindow("Laplacian Threshold Gray", CV_WINDOW_AUTOSIZE);
	cvShowImageWrapper("Laplacian Threshold Gray", grad);

		// change to BW
	temp = createBlankCopy(grad);
	cvSmooth(grad, temp, CV_MEDIAN, 3);
	cvCopy(temp, grad);
	cvReleaseImage(&temp);
	
	cvNamedWindow("Laplacian Threshold Gray 2", CV_WINDOW_AUTOSIZE);
	cvShowImageWrapper("Laplacian Threshold Gray 2", grad);

	cvThreshold(grad, grad, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);
	if(cvGet2D(grad, crop_inside.y, crop_inside.x).val[0] == 255) {
		cvAbsDiffS(grad, grad, cvScalar(255));
	}

		// find the circle containing the given point in the BW image
	findMaxContainedCircle(grad, crop_inside, center, radius, 1);

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

CvPoint2D32f fixPosition(CvPoint2D32f center) {
	CvMemStorage *mem = cvCreateMemStorage();

	char filename[100];
	_snprintf_s(filename, 100, "edges-%d.xml", 0);
	CvSeq* edges = (CvSeq*)cvLoad(filename, mem);

	CvPoint p0 = *(CvPoint*)cvGetSeqElem(edges, 0);
	CvPoint p3 = *(CvPoint*)cvGetSeqElem(edges, 3);
	CvPoint p2 = *(CvPoint*)cvGetSeqElem(edges, 2);

	cvReleaseMemStorage(&mem);

	double X = sqrt(pow((double)p3.x-p2.x,2)+pow((double)p3.y-p2.y,2));
	double Y = sqrt(pow((double)p3.x-p0.x,2)+pow((double)p3.y-p0.y,2));

	double theta = atan2((double)p2.y-p3.y, p2.x-p3.x);
	double alpha = atan2(center.y-p3.y, center.x-p3.x);
	
	double L = sqrt(pow(center.x-p3.x, 2) + pow(center.y-p3.y, 2));

	double A = L*cos(alpha-theta);
	double B = L*sin(alpha-theta);

	CvPoint2D32f res;

	res.x = A/X;
	res.y = 1+B/Y;

	return res;
}