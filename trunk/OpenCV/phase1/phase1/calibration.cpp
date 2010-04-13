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
#include "calibration.h"
#include "balls.h"
#include "misc.h"
#include "VideoCapture.h"
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
				CV_MAT_ELEM(*object_points,float,i,0) = square_height*(j/board_w);
				CV_MAT_ELEM(*object_points,float,i,1) = square_width*(j%board_w);
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
	// We will choose chessboard object points as (r,c):
	// (0,0), (board_w-1,0), (0,board_h-1), (board_w-1,board_h-1).
	//
	CvPoint2D32f objPts[4], imgPts[4];
	objPts[0].x = 0; objPts[0].y = 0;
	objPts[1].x = square_width*(board_w-1); objPts[1].y = 0;
	objPts[2].x = 0; objPts[2].y = square_height*(board_h-1);
	objPts[3].x = square_width*(board_w-1); objPts[3].y = square_height*(board_h-1);
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
	float Z = -10;
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

		key = cvWaitKey();
		if(key == 'u') Z += 0.5;
		if(key == 'd') Z -= 0.5;
	}

	_snprintf_s(filename, 100, "H-%d.xml", device_id);
	cvSave(filename,H); //We can reuse H for the same camera mounting
}

bool took_template = false;

/* this lets the user click on balls and save them as templates */
void grab_templates(CvSize resolution, int device_id) {
	// init camera
	VideoCapture capture(0, resolution.width, resolution.height);
	IplImage *pre_image = capture.CreateCaptureImage();
	IplImage *image = capture.CreateCaptureImage();

	cvNamedWindow("Templates Grabber", CV_WINDOW_AUTOSIZE);

	// load data from files
	char filename[100];
	_snprintf_s(filename, 100, "H-%d.xml", device_id);
	CvMat* H = (CvMat*)cvLoad(filename);

	// initializations
	cvSetMouseCallback("Templates Grabber", &saveTemplateAroundMouse, image);

	// loop
	char c=0;
	while(c != 'q') {
		if(took_template) {
			printf("restarting");
			capture.restart();
			took_template = false;
			cvDestroyWindow("Templates Grabber");
			cvNamedWindow("Templates Grabber", CV_WINDOW_AUTOSIZE);
		}

		capture.waitFrame(pre_image); // capture frame
		cvWarpPerspective(pre_image, image,	H,
			CV_INTER_LINEAR | CV_WARP_INVERSE_MAP | CV_WARP_FILL_OUTLIERS);

		cvShowImage("Templates Grabber", image);
		c=cvWaitKey(1);
	}

	capture.stop();
	cvDestroyWindow("Templates Grabber");

	cvReleaseImage(&image);
	cvReleaseImage(&pre_image);
}

/* mouse callback wrapper for finding a ball around the mouse and saving it
as a template */
void saveTemplateAroundMouse(int event, int x, int y, int flags, void *param) {
	if(event != CV_EVENT_LBUTTONUP)
		return;

	IplImage *img = (IplImage *)param;

	bool is_1600 = true;

	if(is_1600) {
		x /= 1.25;
		y /= 1.172;
	}

	// find the ball parameters
	CvPoint2D32f center;
	float radius;

	/*findBallAround(img, cvPoint(x,y), 50, cvGet2D(img, y, x).val,
		&center, &radius);*/

	center.x = x;
	center.y = y;
	if(is_1600)
		radius = 100;
	else
		radius = 50;

	// crop template
	int crop_size = cvRound(radius) + 1;
	CvRect crop_rect = cvRect(	MAX(cvRound(center.x - crop_size), 0),
		MAX(cvRound(center.y - crop_size), 0), 2*crop_size, 2*crop_size);
	cvSetImageROI(img, crop_rect);

	// save template
	int opt_count = 2;
	char *names[] = {"White", "Red"};
	char *filenames[] = {"white-templ.jpg", "red-templ.jpg"};

	printf("Which ball is it?\n");
	int i;
	for(i = 0; i < opt_count; i++) {
		printf("%d. %s\n", i, names[i]);
	}

	int choice;
	scanf("%d", &choice);
	while(choice<0 || choice>=opt_count)
		scanf("%d", &choice);

	printf("Saved as %s\n", filenames[choice]);

	cvSaveImage(filenames[choice], img);

	took_template = true;
}

void learn_borders(CvSize resolution, int device_id) {
	// init camera
	VideoCapture capture(0, resolution.width, resolution.height);
	IplImage *pre_image = capture.CreateCaptureImage();
	IplImage *image = capture.CreateCaptureImage();
	IplImage *scaled_image = cvCreateImage(cvSize(800, 600),
		image->depth, image->nChannels);

	cvNamedWindow("Borders Marker", CV_WINDOW_AUTOSIZE);

	// load data from files
	char filename[100];
	_snprintf_s(filename, 100, "H-%d.xml", device_id);
	CvMat* H = (CvMat*)cvLoad(filename);

	capture.waitFrame(pre_image); // capture frame
	cvWarpPerspective(pre_image, image,	H,
		CV_INTER_LINEAR | CV_WARP_INVERSE_MAP | CV_WARP_FILL_OUTLIERS);
	if(resolution.width > 800) {
		cvPyrDown(image, scaled_image);
	} else {
		cvCopy(image, scaled_image);
	}

	cvShowImage("Borders Marker", scaled_image);

	// initialize data
	CvMemStorage *mem = cvCreateMemStorage();
	CvSeqWriter writer;
	cvStartWriteSeq(CV_32SC2, sizeof(CvSeq), sizeof(CvPoint), mem, &writer);

	struct border_data data;
	data.img = scaled_image;
	data.resolution = &resolution;
	data.writer = &writer;

	// wait for user to mark
	cvShowImage("Borders Marker", scaled_image);
	cvSetMouseCallback("Borders Marker", &borderPointAroundMouse, &data);
	char c=cvWaitKey(0);

	// save borders
	CvSeq* borders = cvEndWriteSeq(&writer);

	_snprintf_s(filename, 100, "borders-%d.xml", device_id);
	cvSave(filename, borders);

	// release stuff
	capture.stop();
	cvDestroyWindow("Borders Marker");

	cvReleaseMat(&H);
	cvReleaseImage(&image);
	cvReleaseImage(&pre_image);
}

void borderPointAroundMouse(int event, int x, int y, int flags, void *param) {
	struct border_data *data = (struct border_data *)param;

	static CvPoint first_point = cvPoint(-1, -1);
	static CvPoint last_point = cvPoint(-1, -1);
	static CvPoint new_point;
	static IplImage *temp_img = createBlankCopy(data->img);
	static int confirming = 0;

	if(!confirming) {
		if(event == CV_EVENT_LBUTTONUP) {
			new_point = cvPoint(x,y);		

			// draw new image
			cvCopy(data->img, temp_img);
			cvCircle(temp_img, new_point, 1, cvScalar(255, 0, 0), 2);
			if(last_point.x != -1) {
				cvLine(temp_img, last_point, new_point, cvScalar(255, 0, 0), 1);
			} else {
				first_point = new_point;
			}

			cvShowImage("Borders Marker", temp_img);

			confirming = 1;
		} else if(event == CV_EVENT_MBUTTONUP) {
			// connect to the first dot
			cvCopy(data->img, temp_img);
			if(last_point.x != -1) {
				cvLine(temp_img, first_point, new_point, cvScalar(255, 0, 0), 1);
			}

			cvShowImage("Borders Marker", temp_img);

			confirming = 2;
		}
	} else {
		if(event == CV_EVENT_LBUTTONUP && confirming == 1) {
			// add point to sequence
			/*float scale_x = (int)(data->resolution->height/600.);
			float scale_y = (int)(data->resolution->width/800.);*/

			float scale_x = 1;
			float scale_y = 1;

			cout<<"added: ("<<x*scale_x<<", "<<y*scale_y<<")\n";

			CvPoint new_point_scaled = cvPoint(cvRound(x*scale_x), cvRound(y*scale_y)); 

			CV_WRITE_SEQ_ELEM(new_point_scaled, *(data->writer));

			last_point = new_point;
			cvCopy(temp_img, data->img);

			confirming = 0;
		} else if(event == CV_EVENT_RBUTTONUP) {
			cvShowImage("Borders Marker", data->img);

			confirming = 0;
		}
	}
}

void learn_edges(CvSize resolution, int device_id) {
	// init camera
	VideoCapture capture(0, resolution.width, resolution.height);
	IplImage *pre_image = capture.CreateCaptureImage();
	IplImage *image = capture.CreateCaptureImage();
	IplImage *scaled_image = cvCreateImage(cvSize(800, 600),
		image->depth, image->nChannels);

	cvNamedWindow("Edges Marker", CV_WINDOW_AUTOSIZE);

	// load data from files
	char filename[100];
	_snprintf_s(filename, 100, "H-%d.xml", device_id);
	CvMat* H = (CvMat*)cvLoad(filename);

	capture.waitFrame(pre_image); // capture frame
	cvWarpPerspective(pre_image, image,	H,
		CV_INTER_LINEAR | CV_WARP_INVERSE_MAP | CV_WARP_FILL_OUTLIERS);
	if(resolution.width > 800) {
		cvPyrDown(image, scaled_image);
	} else {
		cvCopy(image, scaled_image);
	}

	cvShowImage("Edges Marker", scaled_image);

	// initialize data
	CvMemStorage *mem = cvCreateMemStorage();
	CvSeqWriter writer;
	cvStartWriteSeq(CV_32SC2, sizeof(CvSeq), sizeof(CvPoint), mem, &writer);

	struct border_data data;
	data.img = scaled_image;
	data.resolution = &resolution;
	data.writer = &writer;

	// wait for user to mark
	cvShowImage("Edges Marker", scaled_image);
	cvSetMouseCallback("Edges Marker", &edgePointAroundMouse, &data);
	char c=cvWaitKey(0);

	// save borders
	CvSeq* edges = cvEndWriteSeq(&writer);

	_snprintf_s(filename, 100, "edges-%d.xml", device_id);
	cvSave(filename, edges);

	// release stuff
	capture.stop();
	cvDestroyWindow("Edges Marker");

	cvReleaseMat(&H);
	cvReleaseImage(&image);
	cvReleaseImage(&pre_image);
}

void edgePointAroundMouse(int event, int x, int y, int flags, void *param) {
	struct border_data *data = (struct border_data *)param;

	static CvPoint new_point;
	static IplImage *temp_img = createBlankCopy(data->img);
	static int confirming = 0;

	if(!confirming) {
		if(event == CV_EVENT_LBUTTONUP) {
			new_point = cvPoint(x,y);		

			// draw new image
			cvCopy(data->img, temp_img);
			cvCircle(temp_img, new_point, 1, cvScalar(255, 0, 0), 2);

			cvShowImage("Edges Marker", temp_img);

			confirming = 1;
		}
	} else {
		if(event == CV_EVENT_LBUTTONUP) {
			// add point to sequence
			float scale_x = (int)(data->resolution->height/600.);
			float scale_y = (int)(data->resolution->width/800.);

			cout<<"added: ("<<x*scale_x<<", "<<y*scale_y<<")\n";

			CvPoint new_point_scaled = cvPoint(cvRound(x*scale_x), cvRound(y*scale_y)); 

			CV_WRITE_SEQ_ELEM(new_point_scaled, *(data->writer));

			cvCopy(temp_img, data->img);

			confirming = 0;
		} else if(event == CV_EVENT_RBUTTONUP) {
			cvShowImage("Edges Marker", data->img);

			confirming = 0;
		}
	}
}