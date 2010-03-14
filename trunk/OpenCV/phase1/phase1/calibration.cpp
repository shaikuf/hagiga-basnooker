// calib.cpp
// Calling convention:
// calib board_w board_h number_of_views
//
// Hit 'p' to pause/unpause, ESC to quit
//
#include <cv.h>
#include <highgui.h>
#include <stdio.h>
#include <stdlib.h>
#include "VideoCapture.h"

int n_boards = 0; //Will be set by input list
const int board_dt = 20; //Wait 20 frames per chessboard view
int board_w;
int board_h;

void calibration(int board_w, int board_h, int n_boards, float square_size,
				 CvSize resolution) {

	int board_n = board_w * board_h;
	CvSize board_sz = cvSize( board_w, board_h );

	VideoCapture capture(0,resolution.width,resolution.height);
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

	IplImage *gray_image = cvCreateImage(cvGetSize(image),8,1);//subpixel
	
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
				CV_MAT_ELEM(*object_points,float,i,0) = square_size*(j/board_w);
				CV_MAT_ELEM(*object_points,float,i,1) = square_size*(j%board_w);
				CV_MAT_ELEM(*object_points,float,i,2) = 0.0f;
			}
			CV_MAT_ELEM(*point_counts, int,successes,0) = board_n;
			successes++;
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
	cvSave("Intrinsics.xml",intrinsic_matrix);
	cvSave("Distortion.xml",distortion_coeffs);

	// EXAMPLE OF LOADING THESE MATRICES BACK IN:
	CvMat *intrinsic = (CvMat*)cvLoad("Intrinsics.xml");
	CvMat *distortion = (CvMat*)cvLoad("Distortion.xml");

	// Build the undistort map that we will use for all
	// subsequent frames.
	//
	IplImage* mapx = cvCreateImage( cvGetSize(image), IPL_DEPTH_32F, 1 );
	IplImage* mapy = cvCreateImage( cvGetSize(image), IPL_DEPTH_32F, 1 );
	cvInitUndistortMap(
		intrinsic,
		distortion,
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

//Call:
// birds-eye board_w board_h instrinics distortion image_file
// ADJUST VIEW HEIGHT using keys 'u' up, 'd' down. ESC to quit.
//
int birds_eye() {
	//if(argc != 6) return -1;

	// INPUT PARAMETERS:
	//
	//board_w = atoi(argv[1]);
	board_w = 5;
	//board_h = atoi(argv[2]);
	board_h = 3;
	int board_n = board_w * board_h;
	CvSize board_sz = cvSize( board_w, board_h );
	//CvMat* intrinsic = (CvMat*)cvLoad(argv[3]);
	CvMat* intrinsic = (CvMat*)cvLoad("Intrinsics.xml");
	//CvMat* distortion = (CvMat*)cvLoad(argv[4]);
	CvMat* distortion = (CvMat*)cvLoad("Distortion.xml");
	IplImage* image = 0;
	IplImage* gray_image = 0;
	if( (image = cvLoadImage("C:\\Documents and Settings\\room110\\My Documents\\My Pictures\\Logitech Webcam\\Picture 4.jpg")) == 0 ) {
		printf("Error: Couldn't load %s\n","photo");
		return -1;
	}
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
	cvNamedWindow("Chessboard");
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
		return -1;
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
	// We will choose chessboard object points as (r,c):
	// (0,0), (board_w-1,0), (0,board_h-1), (board_w-1,board_h-1).
	//
	CvPoint2D32f objPts[4], imgPts[4];
	objPts[0].x = 0; objPts[0].y = 0;
	objPts[1].x = board_w-1; objPts[1].y = 0;
	objPts[2].x = 0; objPts[2].y = board_h-1;
	objPts[3].x = board_w-1; objPts[3].y = board_h-1;
	imgPts[0] = corners[0];
	imgPts[1] = corners[board_w-1];
	imgPts[2] = corners[(board_h-1)*board_w];
	imgPts[3] = corners[(board_h-1)*board_w + board_w-1];

	// DRAW THE POINTS in order: B,G,R,YELLOW
	//
	cvCircle( image, cvPointFrom32f(imgPts[0]), 9, CV_RGB(0,0,255), 3);
	cvCircle( image, cvPointFrom32f(imgPts[1]), 9, CV_RGB(0,255,0), 3);
	cvCircle( image, cvPointFrom32f(imgPts[2]), 9, CV_RGB(255,0,0), 3);
	cvCircle( image, cvPointFrom32f(imgPts[3]), 9, CV_RGB(255,255,0), 3);

	// DRAW THE FOUND CHESSBOARD
	//
	cvDrawChessboardCorners(
		image,
		board_sz,
		corners,
		corner_count,
		found
	);

	cvShowImage( "Chessboard", image );

	// FIND THE HOMOGRAPHY
	//
	CvMat *H = cvCreateMat( 3, 3, CV_32F);
	cvGetPerspectiveTransform( objPts, imgPts, H);

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
		printf("showed\n");
		key = cvWaitKey();
		if(key == 'u') Z += 0.5;
		if(key == 'd') Z -= 0.5;
	}

	cvSave("H.xml",H); //We can reuse H for the same camera mounting
	return 0;
}