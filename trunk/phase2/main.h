#ifndef _MAIN_H
#define _MAIN_H

#define NUM_BALLS 8
#define FIND_CUE 1
#define FIND_BALLS 1
#define AUTO_REFETCH 1
#define DRAW_BORDERS 1

/* The main loop of the program */
void gameLoop(CvSize resolution, int device_id);

#endif _MAIN_H