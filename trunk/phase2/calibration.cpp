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

/* this handles computation of the distortion matrices of the camera */
void calibration(int board_w, int board_h, int n_boards, float square_width,
				 float square_height, CvSize resolution, int device_id) {

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

/* this handles generation of perspective wrapping matrix */
void birds_eye(int board_w, int board_h, float square_width, float square_height,
			   CvSize resolution, int device_id) {

	// INPUT PARAMETERS:
	//
	int board_n = board_w * board_h;

	CvSize board_sz = cvSize( board_w, board_h );

	char filename[100];
	_snprintf_s(filename, 100, "Intrinsics-%d.xml", device_id);
	CvMat* intrinsic = (CvMat*)cvLoad(filename);
	_snprintf_s(filename, 100, "Distortion-%d.xml", device_id);
	CvMat* distortion = (CvMat*)cvLoad(filename);

	IplImage* image = 0;
	IplImage* gray_image = 0;

	// CAPTURE IMAGE
	//
	VideoCapture capture(device_id,resolution.width,resolution.height);
	image = capture.CreateCaptureImage();

	cvNamedWindow("Live View");
	char c = 0;
	while(c!='c'){
		capture.waitFrame(image);
		cvShowImage("Live View",image);
		c=cvWaitKey(1);
	}

	capture.stop();
	cvDestroyWindow("Live View");

	gray_image = cvCreateImage( cvGetSize(image), 8, 1 );
	cvCvtColor(image, gray_image, CV_BGR2GRAY );

	// UNDISTORT OUR IMAGE
	//
	IplImage* mapx = cvCreateImage( cvGetSize(image), IPL_DEPTH_32F, 1 );
	IplImage* mapy = cvCreateImage( cvGetSize(image), IPL_DEPTH_32F, 1 );

	//This initializes rectification matrices
	//
	cvInitUndistortMap(
		intrinsic,
		distortion,
		mapx,
		mapy
	);
	IplImage *t = cvCloneImage(image);

	// Rectify our image
	//
	cvRemap( t, image, mapx, mapy );

	// GET THE CHESSBOARD ON THE PLANE
	//
	CvPoint2D32f* corners = new CvPoint2D32f[ board_n ];
	int corner_count = 0;
	int found = cvFindChessboardCorners(
		image,
		board_sz,
		corners,
		&corner_count,
		CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FILTER_QUADS 
	);
	if(!found){
		printf("Couldn't aquire chessboard. only found %d of %d corners\n",
			corner_count,board_n
		);
		birds_eye(board_w, board_h, square_width, square_height, resolution, device_id);
		return;
	}
	//Get Subpixel accuracy on those corners:
	cvFindCornerSubPix(
		gray_image,
		corners,
		corner_count,
		cvSize(11,11),
		cvSize(-1,-1),
		cvTermCriteria( CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 30, 0.1 )
	);

	for(int k=0; k<board_n; k++)
		printf("(%d,%d)\n", (int)corners[k].x, (int)corners[k].y);

	//GET THE IMAGE AND OBJECT POINTS:
	//
	int num_of_points = board_n;
	CvMat *objPts = cvCreateMat(2, num_of_points, CV_32F);
	CvMat *imgPts = cvCreateMat(2, num_of_points, CV_32F);
	for(int i=0; i<board_n; i++) {
		CV_MAT_ELEM(*objPts, float, 0, i) = (i%board_w)*square_width;
		CV_MAT_ELEM(*objPts, float, 1, i) = (i/board_w)*square_height;
		CV_MAT_ELEM(*imgPts, float, 0, i) = corners[i].x;
		CV_MAT_ELEM(*imgPts, float, 1, i) = corners[i].y;
		
		cout<<"matched ("<<CV_MAT_ELEM(*imgPts, float, 0, i)<<","<<\
			CV_MAT_ELEM(*imgPts, float, 1, i)<<") to ("<<\
			CV_MAT_ELEM(*objPts, float, 0, i)<<","<<\
			CV_MAT_ELEM(*objPts, float, 1, i)<<")\n";
	}

	// DRAW THE FOUND CHESSBOARD
	//
	cvDrawChessboardCorners(
		image,
		board_sz,
		corners,
		corner_count,
		found
	);

	cvNamedWindow("Chessboard");
	cvShowImage( "Chessboard", image );

	// FIND THE HOMOGRAPHY
	//
	CvMat *H = cvCreateMat( 3, 3, CV_32F);
	cvFindHomography(objPts, imgPts, H);

	// LET THE USER ADJUST THE Z HEIGHT OF THE VIEW
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

	_snprintf_s(filename, 100, "H-%d.xml", device_id);
	cvSave(filename,H); //We can reuse H for the same camera mounting
}

/* this lets the user click on balls and save them as templates */
void grab_templates(CvSize resolution, int device_id) {
	// init camera
	VideoCapture capture(0, resolution.width, resolution.height);
	IplImage *pre_image = capture.CreateCaptureImage();
	IplImage *image = capture.CreateCaptureImage();

	cvNamedWindow("Templates Grabber", CV_WINDOW_AUTOSIZE);
	cvNamedWindow("New Template", CV_WINDOW_AUTOSIZE);

	// load data from files
	char filename[100];
	_snprintf_s(filename, 100, "H-%d.xml", device_id);
	CvMat* H = (CvMat*)cvLoad(filename);

	// initializations
	cvSetMouseCallback("Templates Grabber", &saveTemplateAroundMouse, image);

	// loop
	char c=0;
	while(c != 'q') {
		capture.waitFrame(pre_image); // capture frame
		cvWarpPerspective(pre_image, image,	H,
			CV_INTER_LINEAR | CV_WARP_INVERSE_MAP | CV_WARP_FILL_OUTLIERS);

		cvShowImage("Templates Grabber", image);
		c=cvWaitKey(1);
	}

	capture.stop();
	cvDestroyWindow("Templates Grabber");
	cvDestroyWindow("New Template");

	cvReleaseImage(&image);
	cvReleaseImage(&pre_image);
}

void saveTemplateAroundMouse(int event, int x, int y, int flags, void *param) {
	if(event != CV_EVENT_LBUTTONUP)
		return;

	IplImage *img = (IplImage *)param;

	fixCoordinates(x, y, cvSize(img->width, img->height));

	// set overlay and crop
	IplImage *templ = overlay_template(img, cvPoint(x,y));
	if(templ == 0)
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

/* this lets the user mark the edges of the projection area */
void learn_edges(bool calibrate, CvSize resolution, int device_id) {
	// init camera
	VideoCapture capture(0, resolution.width, resolution.height);
	IplImage *pre_image = capture.CreateCaptureImage();
	IplImage *image = capture.CreateCaptureImage();

	cvNamedWindow("Edges Marker", CV_WINDOW_AUTOSIZE);

	// load data from files
	char filename[100];
	_snprintf_s(filename, 100, "H-%d.xml", device_id);

	capture.waitFrame(pre_image); // capture frame
	if(!calibrate) {
		CvMat* H = (CvMat*)cvLoad(filename);

		cvWarpPerspective(pre_image, image,	H,
			CV_INTER_LINEAR | CV_WARP_INVERSE_MAP | CV_WARP_FILL_OUTLIERS);

		cvReleaseMat(&H);
	} else {
		cvCopy(pre_image, image);
	}

	cvReleaseImage(&pre_image);

	cvShowImage("Edges Marker", image);

	// initialize data
	CvMemStorage *mem = cvCreateMemStorage();
	CvSeqWriter writer;
	cvStartWriteSeq(CV_32SC2, sizeof(CvSeq), sizeof(CvPoint), mem, &writer);

	struct seq_data data;
	data.img = image;
	data.resolution = &resolution;
	data.writer = &writer;

	// wait for user to mark
	cvShowImage("Edges Marker", image);
	cvSetMouseCallback("Edges Marker", &edgePointAroundMouse, &data);
	char c=cvWaitKey(0);

	CvSeq* edges = cvEndWriteSeq(&writer);

	// release stuff
	capture.stop();
	cvDestroyWindow("Edges Marker");

	if(calibrate) {
		// save the birds-eye matrix
		calibrationFromEdges(edges, resolution, device_id);
	}

	// save borders
	_snprintf_s(filename, 100, "edges-%d.xml", device_id);
	cvSave(filename, edges);

	cvReleaseImage(&image);
}

void edgePointAroundMouse(int event, int x, int y, int flags, void *param) {
	struct seq_data *data = (struct seq_data *)param;

	fixCoordinates(x, y, *(data->resolution));

	static CvPoint new_point;
	static IplImage *temp_img = createBlankCopy(data->img);

	if(event == CV_EVENT_LBUTTONUP) {
		new_point = cvPoint(x,y);		

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

		// add point to sequence
		cout<<"added: ("<<new_point.x<<", "<<new_point.y<<")\n";
		CV_WRITE_SEQ_ELEM(cvPoint(new_point.x, new_point.y), *(data->writer));

		cvShowImage("Edges Marker", data->img);
	}
}

/* this lets the user watch the camera image fixed using the matrices */
void watch(CvSize resolution, bool with_birds_eye, int device_id) {
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

	//This initializes rectification matrices
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

void fixCoordinates(int &x, int &y, CvSize resolution) {
	if(resolution.width = 1600) {
		x = (int)(x/(1600./1280));
		y = (int)(y/(1200./997));
	}
}

/* grab a customized template */
IplImage *overlay_template(IplImage *src, CvPoint center) {
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
		// set overlay and create zoomed version
	tmp = createBlankCopy(overlay);
	tmp_zoom = cvCreateImage(cvSize(overlay->width*2, overlay->height*2),
		overlay->depth, overlay->nChannels);
	if(!hide_overlay)
		cvMul(src, overlay, tmp, 1.0/255);
	else
		cvCopy(src, tmp);
	cvPyrUp(tmp, tmp_zoom);
		// show the preview and release
	cvShowImage("New Template", tmp_zoom);
	cvReleaseImage(&tmp);
	cvReleaseImage(&tmp_zoom);
	cvReleaseImage(&overlay);
	// ==============================

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
			// invert if needed
		if(invert) {
			IplImage *src_i = createBlankCopy(src);
			cvCvtColor(src, src_i, CV_BGR2YCrCb);
			cvNot(src_i, src_i);
			cvCvtColor(src_i, src, CV_YCrCb2BGR);
			cvReleaseImage(&src_i);
		}
			// set overlay and create zoomed version
		tmp = createBlankCopy(overlay);
		tmp_zoom = cvCreateImage(cvSize(overlay->width*2, overlay->height*2),
			overlay->depth, overlay->nChannels);
		if(!hide_overlay)
			cvMul(src, overlay, tmp, 1.0/255);
		else
			cvCopy(src, tmp);
		cvPyrUp(tmp, tmp_zoom);
			// undo invert if needed
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
		// invert if needed
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
		// undo invert if needed
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

/* create a customized overlay */
void setOverlay(IplImage *overlay, int d_width, int d_height, int gradient) {
		// set all black
	cvSet(overlay, cvScalar(0));

		// draw white ellipse
	CvBox2D bounding;
	bounding.center.x = (float)(overlay->width+0.)/2;
	bounding.center.y = (float)(overlay->height+0.)/2;
	bounding.angle = 0;
	bounding.size.width = (float)(overlay->width - d_width);
	bounding.size.height = (float)(overlay->height - d_height);

	cvEllipseBox(overlay, bounding, cvScalar(255,255,255), -1);

		// draw gradient
	for(int j=1; j<=gradient; j++) {
		bounding.size.width += 1;
		bounding.size.height += 1;
		
		int color = (int)((gradient-j+1)*(255/(gradient+1.)));
		cvEllipseBox(overlay, bounding, cvScalar(color,color,color), 1);
	}
}

/* this creates the birds-eye matrix from the edges of the projection image */
void calibrationFromEdges(CvSeq *edges, CvSize resolution, int device_id) {
	float scale = (float)0.10;

	int board_w = 2;
	int board_h = 2;
	int board_n = board_w * board_h;

	IplImage* image = 0;

	// CAPTURE IMAGE
	//
	VideoCapture capture(device_id,resolution.width,resolution.height);
	image = capture.CreateCaptureImage();

	capture.waitFrame(image);
	
	capture.stop();

	cvNamedWindow("Original");
	cvShowImage("Original", image);
	
	//SET THE IMAGE AND OBJECT POINTS:
	//
	int num_of_points = board_n;
	CvMat *objPts = cvCreateMat(2, 4, CV_32F);
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

	// LET THE USER ADJUST THE Z HEIGHT OF THE VIEW
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
	//_snprintf_s(filename, 100, "H-edges-%d.xml", device_id);
	_snprintf_s(filename, 100, "H-%d.xml", device_id);
	cvSave(filename,H); //We can reuse H for the same camera mounting

	// recalculate the edges using the transform
	/*CvMat *imgPtsTrans = cvCreateMat(2, 4, CV_32F);

	cvTransform(imgPts, imgPtsTrans, H);

	((CvPoint*)cvGetSeqElem(edges, 0))->x =
		(int)CV_MAT_ELEM(*imgPtsTrans, float, 0, 0);
	((CvPoint*)cvGetSeqElem(edges, 0))->y =
		(int)CV_MAT_ELEM(*imgPtsTrans, float, 1, 0);

	((CvPoint*)cvGetSeqElem(edges, 1))->x =
		(int)CV_MAT_ELEM(*imgPtsTrans, float, 0, 1);
	((CvPoint*)cvGetSeqElem(edges, 1))->y =
		(int)CV_MAT_ELEM(*imgPtsTrans, float, 1, 1);

	((CvPoint*)cvGetSeqElem(edges, 2))->x =
		(int)CV_MAT_ELEM(*imgPtsTrans, float, 0, 2);
	((CvPoint*)cvGetSeqElem(edges, 2))->y =
		(int)CV_MAT_ELEM(*imgPtsTrans, float, 1, 2);

	((CvPoint*)cvGetSeqElem(edges, 3))->x =
		(int)CV_MAT_ELEM(*imgPtsTrans, float, 0, 3);
	((CvPoint*)cvGetSeqElem(edges, 3))->y =
		(int)CV_MAT_ELEM(*imgPtsTrans, float, 1, 3);*/
}