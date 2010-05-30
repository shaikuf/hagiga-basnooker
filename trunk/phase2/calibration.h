#ifndef _CALIBRATION_H
#define _CALIBRATION_H

// ==============================================
// ==============================================
// CAMERA CALIBRATION

/*	Computes and saves the calibration matrices for the camera, using multiple
	shots of a chessboard
	Gets:
		(int)board_w	the number of rectangles to the width of the chessboard	
		(int)board_h	the number of rectangles to the height of the
						chessboard
		(int)n_boards	the number of chessboard snapshots (has to be above
						some minimum --- check the OpenCV book)
		(float)rect_w	the width of the chessboard rectangles (in cm)
		(float)rect_h	the height of the chessboard rectangles (in cm)
		(CvSize)resolution	the resolution of the camera to use
		(int)device_id	the id of the camera to use	*/
void calibration(int board_w, int board_h, int n_boards, float square_width,
				 float square_height, CvSize resolution, int device_id);

// ==============================================
// ==============================================
// TEMPLATES CALIBRATION

/*	Grab templates of balls to use for later finding
	Gets:
		(CvSize)resolution	the resolution of the camera to use
		(int)device_id	the id of the camera to use	*/
void grabTemplates(CvSize resolution, int device_id);

/*	Mouse handler for grabTemplates
	Gets:
		(void*)param	an IplImage* of the image from which to grab the
						template from */                              
void saveTemplateAroundMouse(int event, int x, int y, int flags, void *param);

/*	Lets the user create a customized template, by starting with an arbitrary
	square around some center point, and then customizing it using the
	keyboard. The keys are:
		i,j,k,l - move the templ. center
		w,a,s,d - change the templ. dimensions
		r - refresh (needs to be done after marking the templ. less wide
		h - show/hide the black overlay
		W,A,S,D - change the overlay dimensions
		c - invert the image (for dark templates -- black ball)
		z,x - give gradient on the edge of the overlay (not used)
	Gets:
		(IplImage*)src	the image to grab the template from
		(CvPoint)center	the center point of the template
	Returns:
		(IplImage*)		the customized template */
IplImage *createTemplateAround(IplImage *src, CvPoint center);

/*	Returns a customized overlay in the given image. The overlay is black
	except for a centered white inner oval whose height is
	[height(overlay)-d_height] and width is [width(overlay)-d_width]. If
	gradient>0 then we surround the inner oval by a number of gradient
	outlines of ovals with colors transitioning uniformly from white to black.
	Gets:
		(IplImage*)overlay	the image to draw the template on
		(int)d_width		the width delta
		(int)d_height		the height delta
		(int)gradient		the gradient length */
void setOverlay(IplImage *overlay, int d_width, int d_height, int gradient);

// ==============================================
// ==============================================
// EDGES CALIBRATION

/*	Lets the user mark the edges of the projection image. If calibrate is false
	we only save them to a file for later use in the processing, and if
	calibrate is true then we use the known size of projection image and the
	marked points to create the perspective transform for our camera.
	Gets:
		(bool)calibrate		whether or not to use these edges to calibrate
							the prespective transform, or just save them
							
		(CvSize)resolution	the resolution of the camera to use
		(int)device_id	the id of the camera to use	*/
void learnEdges(bool calibrate, CvSize resolution, int device_id);

/*	Mouse handler for learnEdges
	Gets:
		(void*)param	a pointer to a seq_data struct containing the image
						to mark the edges on, and a SeqWriter to write the
						edge coordinates to */
void edgePointAroundMouse(int event, int x, int y, int flags, void *param);

/*	Create and save a perspective transform matrix using the edges of the
	projection image. It lets the user customize the transform using the
	keyboard. The keys are:
	u,d - zoom in/out
	i,j,k,l - move the image
	t - flip horizontally
	y - flip vertically
	o,p - rotate the image
	Gets:
		(CvSeq*)edges		a sequence containing the coordinates of the edges
		(CvSize)resolution	the resolution of the camera to use
		(int)device_id		the id of the camera to use */
void calibrationFromEdges(CvSeq *edges, CvSize resolution, int device_id);

// ==============================================
// ==============================================
// MISC

/* this fixes the mouse coordinates according to the image resolution */
/*	Fix the mouse coordinates sent to a mouse handler, according to the image
	resolution.
	Gets:
		(int&)x				the given/returned x coordinate
		(int&)x				the given/returned y coordinate
		(CvSize)resolution	the resolution of the image */
void fixCoordinates(int &x, int &y, CvSize resolution);

/* this lets the user watch the camera image fixed using the matrices */
/*	Let the user watch the camera image after it is transformed using the
	calibration matrices.
	Gets:
		(bool)with_birds_eye	whether or not to use the perspective transform
								(or only the camera calibration)
		(CvSize)resolution	the resolution of the camera to use
		(int)device_id		the id of the camera to use */
void watch(bool with_birds_eye, CvSize resolution, int device_id);

/* A struct used to pass more parameters to mouse handlers */
struct seq_data {
	IplImage *img;
	CvSeqWriter *writer;
	CvSize *resolution;
};

#endif