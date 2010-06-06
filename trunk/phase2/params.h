#ifndef _PARAMS_H
#define _PARAMS_H


// BALLS

// whether or not to run the balls finding in debug mode
#define BALLS_FIND_DEBUG 0
// whether or not to print the found correlation, every time we look for balls
#define BALLS_CORR_DEBUG 1
// the offset outside the calibrated borders that we look for the balls in
#define BOUNDING_RECT_OFFSET 5
// the threshold below the calibrated correlation we want to say that a ball
// was found
//#define BALL_CORR_THD {0.05, 0.05, 0.10, 0.10, 0.10, 0.10, 0.10, 0.10}
#define BALL_CORR_THD {0.05, 0.05, 0.10, 0.10, 0.10, 0.10, 0.10, 0.05}
// the index of the white ball in all of the arrays
#define WHITE_INDEX 1

// CUE

// whether or not to run the cue finding in debug mode
#define CUE_FIND_DEBUG 1

// the minimal R^2 we want when searching for the cue
#define CUE_MIN_COEFF 0.992
// the minimal number of blobs we want when searching for the cue
#define CUE_MIN_BLOBS 2
// the maximal distance of the white ball from the fitted cue line. if it is
// farther, we say that the cue is missing (or just not pointing at the white)
#define CUE_MAX_DIST_FROM_WHITE 20.0
// the threshold value when looking for the white blobs on the cue
#define CUE_THRESH_VAL 220
// the erode number of repetitions when morphologically filtering the
// binary image after the threshold
#define CUE_OPENING_VAL_WHITE 1
#define CUE_OPENING_VAL_BLACK 2
// the dilate number of repetitions
#define CUE_CLOSING_VAL_WHITE 8
#define CUE_CLOSING_VAL_BLACK 6
// the maximal area of a blob when looking for the blobs. we throw away
// bigger ones (in px)
#define CUE_BLOB_MAX_SIZE_WHITE 100
#define CUE_BLOB_MAX_SIZE_BLACK 100
// we throw away blobs farther that this from the table (in px)
#define CUE_BLOB_MAX_DIST_FROM_TABLE_WHITE 40
#define CUE_BLOB_MAX_DIST_FROM_TABLE_BLACK -10
// the maximal distance for a blob from the white (more than that and it's
// ignored)
#define MAX_BLOB_DIST_FROM_WHITE 300
// the averaging time window when smoothing the cue angle
#define CUE_SMOOTH_WINDOWS 7000000 // times 100 ns (=0.7s)
// the ball diameter we draw black on when trying to find the cue
#define BALL_DIAMETER_FOR_CUE_FINDING 35
// linear regression method
//	0. simple linear regression
//	1. total linear regression
#define LINEAR_REGRESSION_METHOD 1

// MAIN

// the number of balls we're searching for
#define NUM_BALLS 8
// whether or not to find the cue
#define FIND_CUE 1
// whether or not to find the balls
#define FIND_BALLS 1
// whether or not to check if the table has changed and send the GUI
// a "refetch" request
#define AUTO_REFETCH 1
// whether or not to fraw the borders on the debug image
#define DRAW_BORDERS 1

// MISC

// whether or not to use the birds-eye fix on the image
#define USE_BIRDS_EYE 1

// an approximate ball diameter (in px)
#define BALL_DIAMETER 30

// whether or not the debug the "is moving" algorithm
#define IS_MOVING_DEBUG 0
// the length of time we check if there has been movement in before checking
// if the white moved
#define IS_MOVING_WINDOW 5000000 // times 100 ns -- 0.5 sec
// the minimal distance that a white has to move for a "refetch" request to
// be sent
#define IS_MOVING_WHITE_DIST 5

// TCP SERVER

// the port we listen for the clients on
#define PORT 1234

#endif