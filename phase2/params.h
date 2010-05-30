#ifndef _PARAMS_H
#define _PARAMS_H


// BALLS

// whether to run the balls finding in debug mode
#define BALLS_FIND_DEBUG 0
// the offset outside the calibrated borders that we look for the balls in
#define BOUNDING_RECT_OFFSET 5

// CUE

// whether to run the cue finding in debug mode
#define CUE_FIND_DEBUG 1

// the minimal R^2 we want when searching for the cue
#define CUE_MIN_COEFF 0.985
// the minimal number of blobs we want when searching for the cue
#define CUE_MIN_BLOBS 2
// the maximal distance of the white ball from the fitted cue line. if it is
// farther, we say that the cue is missing (or just not pointing at the white)
#define CUE_MAX_DIST_FROM_WHITE 20.0
// the threshold value when looking for the white blobs on the cue
#define CUE_THRESH_VAL 230
// the erode number of repetitions when morphologically filtering the
// binary image after the threshold
#define CUE_OPENING_VAL 1
// the dilate number of repetitions
#define CUE_CLOSING_VAL 7
// the maximal area of a blob when looking for the blobs. we throw away
// bigger ones (in px)
#define CUE_BLOB_MAX_SIZE 100
// we throw away blobs farther that this from the table (in px)
#define CUE_BLOB_MAX_DIST_FROM_TABLE 25
// the averaging time window when smoothing the cue angle
#define CUE_SMOOTH_WINDOWS 7000000 // times 100 ns (=0.7s)
// the downsampling factor for sending the cue angle (if it is 3 then
// we send every third computed theta)
#define CUE_THETA_DOWNSAMPLE 1

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
#define IS_MOVING_WINDOW 10000000 // times 100 ns -- 1 sec
// the minimal distance that a white has to move for a "refetch" request to
// be sent
#define IS_MOVING_WHITE_DIST 5

// TCP SERVER

// the port we listen for the clients on
#define PORT 1234

#endif