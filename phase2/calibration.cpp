#include <cv.h>
#include <highgui.h>
#include <stdio.h>
#include <stdlib.h>
#include "calibration.h"
#include "balls.h"
#include "misc.h"
#include "VideoCapture.h"
#include "cue.h"
#include <iostream>

void calibration(int board_w, int board_h, int n_boards, float square_width,
				 float square_height, CvSize resolution, int device_id) {
	 
	/* THIS IS ALMOST AN EXACT COPY OF THE EXAMPLE IN THE OPENCV BOOK.
	 TO FULLY UNDERSTAND IT, READ THE RELEVANT CHAPTER IN THE BOOK */

	int board_n = board_w * board_h;
	CvSize board_sz = cvSize( board_w, board_h );

	VideoCapture capture(device_id,resolution.width,resolution.height);
	IplImage *image = capture.CreateCaptureImage();

	cvNamedWindow( "Calibration" );

	//ALLOCATE STORAGE
	CvMat* image_points = cvCreateMat(n_boards*board_n,2,CV_32FC1);
	CvMat* object_points = cvCreateMat(n_boards*board_n,3,CV_32FC1);
	CvMat* point_counts = cvCreateMat(n_boards,1,CV_32SC1);
	CvMat* intrinsic_matrix = cvCreateMat(3,3,CV_32FC1);
	CvMat* distortion_coeffs = cvCreateMat(5,1,CV_32FC1);

	CvPoint2D32f* corners = new CvPoint2D32f[ board_n ];
	int corner_count;
	int successes = 0;
	int step, frame = 0;

	IplImage *gray_image = cvCreateImage(cvGetSize(image),8,1);
	
	// CAPTURE CORNER VIEWS LOOP UNTIL WE'VE GOT n_boards
	// SUCCESSFUL CAPTURES (ALL CORNERS ON THE BOARD ARE FOUND)
	//
	cvNamedWindow("Live View");
	while(successes < n_boards) {
		char c=0;
		while(c!='c'){
			capture.waitFrame(image);
			cvShowImage("Live View",image);
			c=cvWaitKey(1);
		}

		//==============
		//Find chessboard corners:
		int found = cvFindChessboardCorners(
			image, board_sz, corners, &corner_count,
			CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FILTER_QUADS
			);

		//Get Subpixel accuracy on those corners
		cvCvtColor(image, gray_image, CV_BGR2GRAY);
		cvFindCornerSubPix(gray_image, corners, corner_count,
			cvSize(11,11),cvSize(-1,-1), cvTermCriteria(
			CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 30, 0.1 ));

		//Draw it
		cvDrawChessboardCorners(image, board_sz, corners,
			corner_count, found);
		cvShowImage( "Calibration", image );

		// If we got a good board, add it to our data
		if( corner_count == board_n ) {
			step = successes*board_n;
			for( int i=step, j=0; j<board_n; ++i,++j ) {
				CV_MAT_ELEM(*image_points, float,i,0) = corners[j].x;
				CV_MAT_ELEM(*image_points, float,i,1) = corners[j].y;
				CV_MAT_ELEM(*object_points,float,i,0) = square_height*(j/board_w);
				CV_MAT_ELEM(*object_points,float,i,1) = square_width*(j%board_w);
				CV_MAT_ELEM(*object_points,float,i,2) = 0.0f;
			}
			CV_MAT_ELEM(*point_counts, int,successes,0) = board_n;
			successes++;
		} else {
			printf("only found %d out of %d\n", corner_count, board_n);
		}
		//==============
	} //END COLLECTION WHILE LOOP.

	cvDestroyWindow("Live View");

	//ALLOCATE MATRICES ACCORDING TO HOW MANY CHESSBOARDS FOUND
	CvMat* object_points2 = cvCreateMat(successes*board_n,3,CV_32FC1);
	CvMat* image_points2 = cvCreateMat(successes*board_n,2,CV_32FC1);
	CvMat* point_counts2 = cvCreateMat(successes,1,CV_32SC1);

	//TRANSFER THE POINTS INTO THE CORRECT SIZE MATRICES
	//Below, we write out the details in the next two loops. We could
	//instead have written:
	//image_points->rows = object_points->rows = \
	//successes*board_n; point_counts->rows = successes;
	//
	for(int i = 0; i<successes*board_n; ++i) {
		CV_MAT_ELEM( *image_points2, float, i, 0) =
			CV_MAT_ELEM( *image_points, float, i, 0);
		CV_MAT_ELEM( *image_points2, float,i,1) =
			CV_MAT_ELEM( *image_points, float, i, 1);
		CV_MAT_ELEM(*object_points2, float, i, 0) =
			CV_MAT_ELEM( *object_points, float, i, 0) ;
		CV_MAT_ELEM( *object_points2, float, i, 1) =
			CV_MAT_ELEM( *object_points, float, i, 1) ;
		CV_MAT_ELEM( *object_points2, float, i, 2) =
			CV_MAT_ELEM( *object_points, float, i, 2) ;
	}
	for(int i=0; i<successes; ++i){ //These are all the same number
		CV_MAT_ELEM( *point_counts2, int, i, 0) =
			CV_MAT_ELEM( *point_counts, int, i, 0);
	}
	cvReleaseMat(&object_points);
	cvReleaseMat(&image_points);
	cvReleaseMat(&point_counts);

	// At this point we have all of the chessboard corners we need.
	// Initialize the intrinsic matrix such that the two focal
	// lengths have a ratio of 1.0
	//
	CV_MAT_ELEM( *intrinsic_matrix, float, 0, 0 ) = 1.0f;
	CV_MAT_ELEM( *intrinsic_matrix, float, 1, 1 ) = 1.0f;

	//CALIBRATE THE CAMERA!
	cvCalibrateCamera2(
		object_points2, image_points2,
		point_counts2, cvGetSize( image ),
		intrinsic_matrix, distortion_coeffs,
		NULL, NULL,0 //CV_CALIB_FIX_ASPECT_RATIO
	);

	// SAVE THE INTRINSICS AND DISTORTIONS
	char filename[100];
	_snprintf_s(filename, 100, "Intrinsics-%d.xml", device_id);
	cvSave(filename,intrinsic_matrix);
	_snprintf_s(filename, 100, "Distortion-%d.xml", device_id);
	cvSave(filename,distortion_coeffs);

	// Build the undistort map that we will use for all
	// subsequent frames.
	//
	IplImage* mapx = cvCreateImage( cvGetSize(image), IPL_DEPTH_32F, 1 );
	IplImage* mapy = cvCreateImage( cvGetSize(image), IPL_DEPTH_32F, 1 );
	cvInitUndistortMap(
		intrinsic_matrix,
		distortion_coeffs,
		mapx,
		mapy
	);

	// Just run the camera to the screen, now showing the raw and
	// the undistorted image.
	//
	cvNamedWindow( "Undistort" );
	while(image) {
		IplImage *t = cvCloneImage(image);
		cvShowImage( "Calibration", image ); // Show raw image
		cvRemap( t, image, mapx, mapy ); // Undistort image
		cvReleaseImage(&t);
		cvShowImage("Undistort", image); // Show corrected image

		//Handle pause/unpause and ESC
		int c = cvWaitKey(15);
		if(c == 'p') {
			c = 0;
			while(c != 'p' && c != 27) {
				c = cvWaitKey(250);
			}
		}
		if(c == 27)
			break;
		capture.waitFrame(image);
	}
}


void grabTemplates(CvSize resolution, int device_id) {
	// init camera
	VideoCapture capture(0, resolution.width, resolution.height);
	IplImage *pre_image = capture.CreateCaptureImage();
	IplImage *image = capture.CreateCaptureImage();

	cvNamedWindow("Templates Grabber", CV_WINDOW_AUTOSIZE);
	cvNamedWindow("New Template", CV_WINDOW_AUTOSIZE);

	// load the perspective transform from file
	char filename[100];
	_snprintf_s(filename, 100, "H-%d.xml", device_id);
	CvMat* H = (CvMat*)cvLoad(filename);

	// register our mouse handler
	cvSetMouseCallback("Templates Grabber", &saveTemplateAroundMouse, image);

	// capture new frames until a ball is clicked and then the mouse handler
	// is invoked, until 'q' is pressed
	char c=0;
	while(c != 'q') {
		capture.waitFrame(pre_image); // capture frame
		cvWarpPerspective(pre_image, image,	H,
			CV_INTER_LINEAR | CV_WARP_INVERSE_MAP | CV_WARP_FILL_OUTLIERS);

		cvShowImage("Templates Grabber", image);
		c=cvWaitKey(1);
	}

	// release the camera and allocated stuff
	capture.stop();
	cvDestroyWindow("Templates Grabber");
	cvDestroyWindow("New Template");

	cvReleaseImage(&image);
	cvReleaseImage(&pre_image);
}

void saveTemplateAroundMouse(int event, int x, int y, int flags, void *param) {
	// only respond to LMB
	if(event != CV_EVENT_LBUTTONUP)
		return;

	// read the sent parameters
	IplImage *img = (IplImage *)param;

	// fix the received mouse coordinates
	fixCoordinates(x, y, cvSize(img->width, img->height));

	// create a customized template with the clicked point as its
	// starting center point
	IplImage *templ = createTemplateAround(img, cvPoint(x,y));

	if(templ == 0) // 
		return;
	
	// save template
	int opt_count = 8;
	char *names[] = {"White", "Red", "Blue", "Yellow", "Green", "Pink", "Brown",
		"Black"};
	char *filenames[] = {"white-templ.jpg", "red-templ.jpg", "blue-templ.jpg",
		"yellow-templ.jpg", "green-templ.jpg", "pink-templ.jpg", "brown-templ.jpg",
		"black-templ.jpg"};

	printf("Which ball is it?\n");
	int i;
	for(i = 0; i < opt_count; i++) {
		printf("%d. %s\n", i, names[i]);
	}
	printf("-1. Reject image\n");

	int choice;
	scanf("%d", &choice);
	while(choice<-1 || choice>=opt_count)
		scanf("%d", &choice);

	if(choice != -1) {
		printf("Saved as %s\n", filenames[choice]);
		cvSaveImage(filenames[choice], templ);
	} else {
		printf("Rejected\n");
	}

	// clear the ROI we set
	cvResetImageROI(img);

	// release stuff
	cvReleaseImage(&templ);
}

IplImage *createTemplateAround(IplImage *src, CvPoint center) {
	cvNamedWindow("New Template", CV_WINDOW_AUTOSIZE);

	IplImage* overlay, *tmp, *tmp_zoom;

	// parameters of the transformation
		// crop
	int width = BALL_DIAMETER;
	int height = BALL_DIAMETER;
	int x = center.x-width/2;
	int y = center.y-height/2;

		// overlay
	int d_width = 0, d_height = 0;
	int gradient = 0;

		// general
	int hide_overlay = 1;
	int invert = 0;

	// ==== create preview image ====
		// crop
	cvSetImageROI(src, cvRect(x, y, width, height));
		// create overlay image
	overlay = createBlankCopy(src);
	setOverlay(overlay, d_width, d_height, gradient);
		// set overlay
	tmp = createBlankCopy(overlay);
	if(!hide_overlay)
		cvMul(src, overlay, tmp, 1.0/255);
	else
		cvCopy(src, tmp);
		// create zoomed version (we display an x2 zoom of the template for
		// more percision)
	tmp_zoom = cvCreateImage(cvSize(overlay->width*2, overlay->height*2),
		overlay->depth, overlay->nChannels);
	cvPyrUp(tmp, tmp_zoom);
		// show the preview and release
	cvShowImage("New Template", tmp_zoom);
	cvReleaseImage(&tmp);
	cvReleaseImage(&tmp_zoom);
	cvReleaseImage(&overlay);
	// ==============================

	// while esc was not pressed, let the user customize the template
	char key = 0;
	while(key != 27) {
		key = cvWaitKey(0);

		switch(key) {
			case 'i': // move the crop up
				y--;
				break;
			case 'k': // move the crop down
				y++;
				break;
			case 'j': // move the crop left
				x--;
				break;
			case 'l': // move the crop right
				x++;
				break;
			case 'a': // crop less wide
				width--;
				break;
			case 'd': // crop wider
				width++;
				break;
			case 'w': // crop less tall
				height--;
				break;
			case 's': // crop taller
				height++;
				break;
			case 'h': // hide the overlay
				hide_overlay = 1-hide_overlay;
				break;
			case 'A': // overlay less wide
				d_width--;
				break;
			case 'D': // overlay wider
				d_width++;
				break;
			case 'W': // overlay less tall
				d_height--;
				break;
			case 'S': // overlay taller
				d_height++;
				break;
			case 'z': // overlay less transparent
				if(gradient > 0)
					gradient--;
				break;
			case 'x': // overlay more transparent
				gradient++;
				break;
			case 'c': // invert image
				invert = 1-invert;
				break;
			case 'r': // refresh window:
				cvDestroyWindow("New Template");
				cvNamedWindow("New Template", CV_WINDOW_AUTOSIZE);
				break;
		}


		// ==== create preview image ====
			// crop
		cvSetImageROI(src, cvRect(x, y, width, height));
			// create overlay image
		overlay = createBlankCopy(src);
		setOverlay(overlay, d_width, d_height, gradient);
			// invert if needed (on the original image)
		if(invert) {
			IplImage *src_i = createBlankCopy(src);
			cvCvtColor(src, src_i, CV_BGR2YCrCb);
			cvNot(src_i, src_i);
			cvCvtColor(src_i, src, CV_YCrCb2BGR);
			cvReleaseImage(&src_i);
		}
			// set overlay
		tmp = createBlankCopy(overlay);
		if(!hide_overlay)
			cvMul(src, overlay, tmp, 1.0/255);
		else
			cvCopy(src, tmp);
			// create zoomed version
		tmp_zoom = cvCreateImage(cvSize(overlay->width*2, overlay->height*2),
			overlay->depth, overlay->nChannels);
		cvPyrUp(tmp, tmp_zoom);
			// undo invert if needed (on the original image)
		if(invert) {
			IplImage *src_i = createBlankCopy(src);
			cvCvtColor(src, src_i, CV_BGR2YCrCb);
			cvNot(src_i, src_i);
			cvCvtColor(src_i, src, CV_YCrCb2BGR);
			cvReleaseImage(&src_i);
		}
			// show the preview and release
		cvShowImage("New Template", tmp_zoom);
			// release temp images
		cvReleaseImage(&tmp);
		cvReleaseImage(&tmp_zoom);
		cvReleaseImage(&overlay);
		// ==============================
	}

	// ==== create final template ====
		// crop
	cvSetImageROI(src, cvRect(x, y, width, height));
		// create overlay image
	overlay = createBlankCopy(src);
	setOverlay(overlay, d_width, d_height, gradient);
		// invert if needed (on the original image)
	if(invert) {
		IplImage *src_i = createBlankCopy(src);
		cvCvtColor(src, src_i, CV_BGR2YCrCb);
		cvNot(src_i, src_i);
		cvCvtColor(src_i, src, CV_YCrCb2BGR);
		cvReleaseImage(&src_i);
	}
		// set overlay and create zoomed version
	tmp = createBlankCopy(overlay);
	cvMul(src, overlay, tmp, 1.0/255);
		// undo invert if needed (on the original image)
	if(invert) {
		IplImage *src_i = createBlankCopy(src);
		cvCvtColor(src, src_i, CV_BGR2YCrCb);
		cvNot(src_i, src_i);
		cvCvtColor(src_i, src, CV_YCrCb2BGR);
		cvReleaseImage(&src_i);
	}
		// release temp images
	cvReleaseImage(&overlay);
	// ===============================

	// release stuff
	cvDestroyWindow("New Template");

	return tmp;
}

void setOverlay(IplImage *overlay, int d_width, int d_height, int gradient) {
		// set all black
	cvSet(overlay, cvScalar(0));

		// draw the white ellipse
	CvBox2D bounding;
	bounding.center.x = (float)(overlay->width+0.)/2;
	bounding.center.y = (float)(overlay->height+0.)/2;
	bounding.angle = 0;
	bounding.size.width = (float)(overlay->width - d_width);
	bounding.size.height = (float)(overlay->height - d_height);

	cvEllipseBox(overlay, bounding, cvScalar(255,255,255), -1);

		// draw the gradient
	for(int j=1; j<=gradient; j++) {
		bounding.size.width += 1;
		bounding.size.height += 1;
		
		int color = (int)((gradient-j+1)*(255/(gradient+1.)));
		cvEllipseBox(overlay, bounding, cvScalar(color,color,color), 1);
	}
}

void learnEdges(bool calibrate, CvSize resolution, int device_id) {
	// init camera
	VideoCapture capture(0, resolution.width, resolution.height);
	IplImage *pre_image = capture.CreateCaptureImage();
	IplImage *image = capture.CreateCaptureImage();

	cvNamedWindow("Edges Marker", CV_WINDOW_AUTOSIZE);

	// capture a frame
	capture.waitFrame(pre_image);

	// fix the perspective on the image, if we're not calibrating
	if(!calibrate) {
		char filename[100];
		_snprintf_s(filename, 100, "H-%d.xml", device_id);

		CvMat* H = (CvMat*)cvLoad(filename);

		cvWarpPerspective(pre_image, image,	H,
			CV_INTER_LINEAR | CV_WARP_INVERSE_MAP | CV_WARP_FILL_OUTLIERS);

		cvReleaseMat(&H);
	} else {
		cvCopy(pre_image, image);
	}

	// show the image
	cvShowImage("Edges Marker", image);

	// initialize data structures
	CvMemStorage *mem = cvCreateMemStorage();
	CvSeqWriter writer;
	cvStartWriteSeq(CV_32SC2, sizeof(CvSeq), sizeof(CvPoint), mem, &writer);

	struct seq_data data;
	data.img = image;
	data.resolution = &resolution;
	data.writer = &writer;

	// show the captured image
	cvShowImage("Edges Marker", image);

	// register the mouse callback, and let the user mark until he
	// presses a key
	cvSetMouseCallback("Edges Marker", &edgePointAroundMouse, &data);
	char c=cvWaitKey(0);

	// create a CvSeq out of the selected points
	CvSeq* edges = cvEndWriteSeq(&writer);

	// release the camera
	capture.stop();
	cvDestroyWindow("Edges Marker");

	if(calibrate) {
		// create and save the perspective transform matrix from the edges
		calibrationFromEdges(edges, resolution, device_id);
	} else {
		// save the edges positions
		char filename[100];
		_snprintf_s(filename, 100, "edges-%d.xml", device_id);
		cvSave(filename, edges);
	}

	// release stuff
	cvReleaseImage(&pre_image);
	cvReleaseImage(&image);
}

void edgePointAroundMouse(int event, int x, int y, int flags, void *param) {
	// only respond to LMB
	if(event != CV_EVENT_LBUTTONUP)
		return;

	// recover the sent data
	struct seq_data *data = (struct seq_data *)param;
	IplImage *temp_img = createBlankCopy(data->img);

	// fix the received coordinates
	fixCoordinates(x, y, *(data->resolution));
	CvPoint new_point = cvPoint(x,y);

	// let the user move the marking
	char key = 0;
	do {
		switch(key) {
			case 'i':
				new_point.y -= 1;
				break;
			case 'k':
				new_point.y += 1;
				break;
			case 'j':
				new_point.x -= 1;
				break;
			case 'l':
				new_point.x += 1;
				break;
		}

		// draw new image
		cvCopy(data->img, temp_img);
		cvCircle(temp_img, new_point, 1, cvScalar(255, 0, 0), 2);

		cvShowImage("Edges Marker", temp_img);

		key = cvWaitKey();
	} while(key != 27);

	// draw final image
	cvCircle(data->img, new_point, 1, cvScalar(0, 255, 0), 2);

	cvShowImage("Edges Marker", data->img);

	// add the edge point to the sequence
	cout<<"added: ("<<new_point.x<<", "<<new_point.y<<")\n";
	CV_WRITE_SEQ_ELEM(cvPoint(new_point.x, new_point.y), *(data->writer));

	// release stuff
	cvReleaseImage(&temp_img);
}

void calibrationFromEdges(CvSeq *edges, CvSize resolution, int device_id) {
	// CAPTURE IMAGE
	//
	VideoCapture capture(device_id,resolution.width,resolution.height);
	IplImage *image = capture.CreateCaptureImage();

	capture.waitFrame(image);
	
	capture.stop();

	cvNamedWindow("Original");
	cvShowImage("Original", image);
	
	//SET THE IMAGE AND OBJECT POINTS:
	//
		// a factor for scaling the projection image size in px
	float scale = (float)0.10;

		// the dimensions of the edges matrix
	int board_w = 2;
	int board_h = 2;
	int board_n = board_w * board_h;

	int num_of_points = board_n;
		// the coordinates of the edges in the table plane
	CvMat *objPts = cvCreateMat(2, 4, CV_32F);
		// the coordinates of the edges in the image plane
	CvMat *imgPts = cvCreateMat(2, 4, CV_32F);

		// obj pts
	CV_MAT_ELEM(*objPts, float, 0, 0) = scale*0;
	CV_MAT_ELEM(*objPts, float, 1, 0) = scale*0;

	CV_MAT_ELEM(*objPts, float, 0, 1) = scale*820;
	CV_MAT_ELEM(*objPts, float, 1, 1) = scale*0;

	CV_MAT_ELEM(*objPts, float, 0, 2) = scale*820;
	CV_MAT_ELEM(*objPts, float, 1, 2) = scale*375;

	CV_MAT_ELEM(*objPts, float, 0, 3) = scale*0;
	CV_MAT_ELEM(*objPts, float, 1, 3) = scale*375;

		// img pts
	CV_MAT_ELEM(*imgPts, float, 0, 0) =
		(float)((CvPoint*)cvGetSeqElem(edges,0))->x;
	CV_MAT_ELEM(*imgPts, float, 1, 0) = 
		(float)((CvPoint*)cvGetSeqElem(edges, 0))->y;

	CV_MAT_ELEM(*imgPts, float, 0, 1) = 
		(float)((CvPoint*)cvGetSeqElem(edges, 1))->x;
	CV_MAT_ELEM(*imgPts, float, 1, 1) = 
		(float)((CvPoint*)cvGetSeqElem(edges, 1))->y;

	CV_MAT_ELEM(*imgPts, float, 0, 2) = 
		(float)((CvPoint*)cvGetSeqElem(edges, 2))->x;
	CV_MAT_ELEM(*imgPts, float, 1, 2) = 
		(float)((CvPoint*)cvGetSeqElem(edges, 2))->y;

	CV_MAT_ELEM(*imgPts, float, 0, 3) = 
		(float)((CvPoint*)cvGetSeqElem(edges, 3))->x;
	CV_MAT_ELEM(*imgPts, float, 1, 3) = 
		(float)((CvPoint*)cvGetSeqElem(edges, 3))->y;

	// FIND THE HOMOGRAPHY
	//
	CvMat *H = cvCreateMat( 3, 3, CV_32F);
	cvFindHomography(objPts, imgPts, H);

	// LET THE USER ADJUST THE PARAMETERS OF THE HOMOGRAPHY
	//
	float Z = 1;
	int key = 0;
	IplImage *birds_image = cvCloneImage(image);
	cvNamedWindow("Birds_Eye");

	// LOOP TO ALLOW USER TO PLAY WITH HEIGHT:
	//
	// escape key stops
	//
	while(key != 27) {
		// Set the height
		//
		CV_MAT_ELEM(*H,float,2,2) = Z;
		// COMPUTE THE FRONTAL PARALLEL OR BIRD'S-EYE VIEW:
		// USING HOMOGRAPHY TO REMAP THE VIEW
		//
		cvWarpPerspective(
			image,
			birds_image,
			H,
			CV_INTER_LINEAR | CV_WARP_INVERSE_MAP | CV_WARP_FILL_OUTLIERS
		);
		cvShowImage( "Birds_Eye", birds_image );

		float z_delta = 0.5;
		float theta_delta = (float)(0.02*PI/180);
		int movement_delta = 25;
		CvPoint2D32f tmp;
		key = cvWaitKey();
		switch(key) {
			case 'u': // up on the Z axis
				Z += z_delta;
				break;
			case 'd': // down on the Z axis
				Z -= z_delta;
				break;
			case 'i': // up on X-Y
				for(int i=0; i<num_of_points; i++)
					CV_MAT_ELEM(*objPts, float, 1, i) += movement_delta;
				cvFindHomography(objPts, imgPts, H);
				break;
			case 'j': // left on X-Y
				for(int i=0; i<num_of_points; i++)
					CV_MAT_ELEM(*objPts, float, 0, i) += movement_delta;
				cvFindHomography(objPts, imgPts, H);
				break;
			case 'k': // down on X-Y
				for(int i=0; i<num_of_points; i++)
					CV_MAT_ELEM(*objPts, float, 1, i) -= movement_delta;
				cvFindHomography(objPts, imgPts, H);
				break;
			case 'l': // right on X-Y
				for(int i=0; i<num_of_points; i++)
					CV_MAT_ELEM(*objPts, float, 0, i) -= movement_delta;
				cvFindHomography(objPts, imgPts, H);
				break;
			case 't': // flip horizontally
				for(int i=0; i<board_h; i++) {
					for(int j=0; j<board_w/2; j++) {
						tmp.x = CV_MAT_ELEM(*objPts, float, 0, j*board_w+i);
						tmp.y = CV_MAT_ELEM(*objPts, float, 1, j*board_w+i);

						CV_MAT_ELEM(*objPts, float, 0, j*board_w+i) = \
							CV_MAT_ELEM(*objPts, float, 0, j*(board_w+1)-i-1);
						CV_MAT_ELEM(*objPts, float, 1, j*board_w+i) = \
							CV_MAT_ELEM(*objPts, float, 1, j*(board_w+1)-i-1);

						CV_MAT_ELEM(*objPts, float, 0, j*(board_w+1)-i-1) = tmp.x;
						CV_MAT_ELEM(*objPts, float, 1, j*(board_w+1)-i-1) = tmp.y;
					}
				}
				break;
			case 'y': // flip vertically
				for(int j=0; j<board_w; j++) {
					for(int i=0; i<board_h/2; i++) {
						tmp.x = CV_MAT_ELEM(*objPts, float, 0, j*board_w+i);
						tmp.y = CV_MAT_ELEM(*objPts, float, 1, j*board_w+i);

						CV_MAT_ELEM(*objPts, float, 0, j*board_w+i) = \
							CV_MAT_ELEM(*objPts, float, 0, (board_h-j-1)*board_w+i);
						CV_MAT_ELEM(*objPts, float, 1, j*board_w+i) = \
							CV_MAT_ELEM(*objPts, float, 1, (board_h-j-1)*board_w+i);

						CV_MAT_ELEM(*objPts, float, 0, (board_h-j-1)*board_w+i) = tmp.x;
						CV_MAT_ELEM(*objPts, float, 1, (board_h-j-1)*board_w+i) = tmp.y;
					}
				}
				break;
			case 'o':
				// rotate clockwise
				for(int i=0; i<num_of_points; i++) {
					tmp.x = CV_MAT_ELEM(*objPts, float, 0, i);
					tmp.y = CV_MAT_ELEM(*objPts, float, 1, i);
					CV_MAT_ELEM(*objPts, float, 0, i) = tmp.x*cos(theta_delta)\
						- tmp.y*sin(theta_delta);
					CV_MAT_ELEM(*objPts, float, 1, i) = tmp.x*sin(theta_delta)\
						+ tmp.y*cos(theta_delta);
				}
				cvFindHomography(objPts, imgPts, H);
				break;
			case 'p':
				// rotate counter-clockwise
				for(int i=0; i<num_of_points; i++) {
					tmp.x = CV_MAT_ELEM(*objPts, float, 0, i);
					tmp.y = CV_MAT_ELEM(*objPts, float, 1, i);
					CV_MAT_ELEM(*objPts, float, 0, i) = tmp.x*cos(-1*theta_delta)\
						- tmp.y*sin(-1*theta_delta);
					CV_MAT_ELEM(*objPts, float, 1, i) = tmp.x*sin(-1*theta_delta)\
						+ tmp.y*cos(-1*theta_delta);
				}
				cvFindHomography(objPts, imgPts, H);
				break;
		}
	}

	char filename[100];
	_snprintf_s(filename, 100, "H-%d.xml", device_id);
	cvSave(filename,H); //We can reuse H for the same camera mounting
}

void calibrateCorrelationThds(CvSize resolution, int device_id) {
	// init camera
	VideoCapture capture(0, resolution.width, resolution.height);
	IplImage *pre_image = capture.CreateCaptureImage();
	IplImage *image = capture.CreateCaptureImage();

	// capture a frame
	capture.waitFrame(pre_image);

	// fix the perspective on the image, if we're not calibrating
	char filename[100];
	_snprintf_s(filename, 100, "H-%d.xml", device_id);

	CvMat* H = (CvMat*)cvLoad(filename);

	cvWarpPerspective(pre_image, image,	H,
		CV_INTER_LINEAR | CV_WARP_INVERSE_MAP | CV_WARP_FILL_OUTLIERS);

	cvReleaseMat(&H);

	// find the ball correlations
	int ball_counts[8] = {1,1,1,1,1,1,1,1};
	int ball_thd[8] = {0,0,0,0,0,0,0,1};
	double ball_corr[8];
	findBallCorrelations(image, ballTemplates(), ballInverseTempls(),
		ball_corr, NUM_BALLS);


	// save to file
	CvMat *corr_thds = cvCreateMat(1, 8, CV_32F);

	for(int i=0; i<NUM_BALLS; i++) {
		CV_MAT_ELEM(*corr_thds, float, 0, i) = (float)ball_corr[i];
	}

	cvSave("corr-thd.xml", corr_thds);
}

void fixCoordinates(int &x, int &y, CvSize resolution) {
	// this is some ampiric fix i found
	if(resolution.width = 1600) {
		x = (int)(x/(1600./1280));
		y = (int)(y/(1200./997));
	}
}

void watch(bool with_birds_eye, CvSize resolution, int device_id) {
	char filename[100];
	_snprintf_s(filename, 100, "Intrinsics-%d.xml", device_id);
	CvMat* intrinsic = (CvMat*)cvLoad(filename);
	_snprintf_s(filename, 100, "Distortion-%d.xml", device_id);
	CvMat* distortion = (CvMat*)cvLoad(filename);

	IplImage* image = 0;

	// CAPTURE IMAGE
	//
	VideoCapture capture(device_id,resolution.width,resolution.height);
	image = capture.CreateCaptureImage();

	// UNDISTORT OUR IMAGE
	//
	IplImage* mapx = cvCreateImage( cvGetSize(image), IPL_DEPTH_32F, 1 );
	IplImage* mapy = cvCreateImage( cvGetSize(image), IPL_DEPTH_32F, 1 );

	// This initializes rectification matrices
	//
	cvInitUndistortMap(
		intrinsic,
		distortion,
		mapx,
		mapy
	);
	IplImage *t = createBlankCopy(image);

	CvMat* H;
	if(with_birds_eye) {
		char filename[100];
		_snprintf_s(filename, 100, "H-%d.xml", device_id);
		H = (CvMat*)cvLoad(filename);
	}

	// LIVE VIEW

	cvNamedWindow("Live View");
	char c = 0;
	while(c!=27){
		capture.waitFrame(image);

		cvCopy(image, t);
		if(with_birds_eye) {
			cvWarpPerspective(t, image,	H,
				CV_INTER_LINEAR | CV_WARP_INVERSE_MAP | CV_WARP_FILL_OUTLIERS);
		} else {
			cvRemap( t, image, mapx, mapy );
		}

		cvShowImage("Live View",image);

		c = cvWaitKey(10);
	}

	capture.stop();
	cvDestroyWindow("Live View");
}



void calibrateHoles(CvSize resolution, int device_id){
	// init camera
	VideoCapture capture(0, resolution.width, resolution.height);
	IplImage *pre_image = capture.CreateCaptureImage();
	IplImage *image = capture.CreateCaptureImage();


	cvNamedWindow("Holes", CV_WINDOW_AUTOSIZE);

	// capture a frame
	capture.waitFrame(pre_image);

	// fix the perspective on the image
	char filename[100];
	_snprintf_s(filename, 100, "H-%d.xml", device_id);
	CvMat* H = (CvMat*)cvLoad(filename);

	cvWarpPerspective(pre_image, image,	H,
		CV_INTER_LINEAR | CV_WARP_INVERSE_MAP | CV_WARP_FILL_OUTLIERS);

	cvReleaseMat(&H);

	//initialize the circles positions
	CvPoint p[6];
	int radius[6];
	CvMat* holes = (CvMat*)cvLoad("Hole.xml");

	if(holes == 0) {
		// this loads the default holes from the edges file
		CvSeq *borders = tableBorders();

		p[0] = *(CvPoint*)cvGetSeqElem(borders, 0); // top-left
		p[2] = *(CvPoint*)cvGetSeqElem(borders, 1); // top-right
		p[3] = *(CvPoint*)cvGetSeqElem(borders, 2); // bottom-right
		p[5] = *(CvPoint*)cvGetSeqElem(borders, 3); // bottom-left
		
		p[1] = cvPoint((p[0].x + p[2].x)/2, (p[0].y + p[2].y)/2);
			// top-center
		p[4] = cvPoint((p[5].x + p[3].x)/2, (p[5].y + p[3].y)/2);
			// bottom-center

		radius[0] = 30; radius[1] = 30; radius[2] = 30;
		radius[3] = 30; radius[4] = 30; radius[5] = 30;

		// allocate a new holes matrix
		CvMat *holes = cvCreateMat(6, 3, CV_32S);

	} else {
		for(int i=0; i<6; i++){
			p[i].x = CV_MAT_ELEM(*holes,int,i,0);
			p[i].y = CV_MAT_ELEM(*holes,int,i,1);
			radius[i] = CV_MAT_ELEM(*holes,int,i,2);
		}
	}
	
	//draw the circles for the first time
	IplImage* cpy_image = cvCloneImage(image);
	for(int i=0; i<6; i++) {
		cvCircle(cpy_image, p[i], radius[i], cvScalar(0), -1);
	}

	cvShowImage("Holes", cpy_image);
	cvReleaseImage(&cpy_image);

	// let the user customize them
	char c=0;
	cout<<"choose a hole to fix:\n1:top-left\n2:top-center\n"
			<<"3:top-right\n4:bottom-right\n5:bottom-center\n"
			<<"6:bottom-left"<<endl;
	c = cvWaitKey(0)-'1';
	while(c>=0 && c<6) {
		drawHolesOnImage(image, c, p, radius);
		c = cvWaitKey(0)-'1';
	}

	// save to file
	for(int i=0; i<6; i++) {
		CV_MAT_ELEM(*holes,int,i,0) = p[i].x;
		CV_MAT_ELEM(*holes,int,i,1) = p[i].y;
		CV_MAT_ELEM(*holes,int,i,2) = radius[i];
	}

	cvSave("Hole.xml", holes);

	// release stuff
	cvReleaseImage(&image);
	cvReleaseImage(&pre_image);
}

void drawHolesOnImage(IplImage *image, int index, CvPoint p[], int r[]) {
	// temp image we paint on
	IplImage *cpy_image = createBlankCopy(image);

	// how much to much per keypress
	int delta_x=1, delta_y=1;
	
	int key = cvWaitKey();
	while(key!=27){//while 'esc' not pressed
		switch(key){
			case 'i'://move up
				p[index].y -= delta_y;
				break;
			case 'k'://move down
				p[index].y += delta_y;
				break;
			case 'j'://move left
				p[index].x -= delta_x;
				break;
			case 'l'://move right
				p[index].x += delta_x;
				break;
			case 'a'://increase radius
				r[index] += 1;
				break;
			case 's'://decrease radius
				r[index] -= 1;
				break;
		}

		//draw the circles
		cvCopy(image,cpy_image);
		for(int i=0; i<6; i++)
			cvCircle(cpy_image, p[i], r[i], cvScalar(0), -1);

		cvShowImage("Holes",cpy_image);

		key = cvWaitKey(0);
	}
}